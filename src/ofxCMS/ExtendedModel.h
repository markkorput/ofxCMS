#pragma once

#include "ofxLambdaEvent/function.h" // for FUNCTION macro
#include "Model.h"

namespace ofxCMS {
    //! transformer that actively (monitoring for realtime change) transforms an specified ofxCMS::Model instance's attribute through a lambda (provided by owner)
    class ValueTransformer {
        public:
            typedef FUNCTION<void(const string&)> ValueFunctor;

        public:
            ValueTransformer() : model(NULL){}
            ~ValueTransformer(){ destroy(); }

            //! convenience method for registering transformer on only one attribute (which is probably most of the time the case)
            void setup(Model &model, const string& attr, ValueFunctor func){
                std::vector<string> attrs;
                attrs.push_back(attr);
                setup(model, attrs, func);
            }

            //! register this transformer on the specified model for all the attributes in attrs, using the given func lambda for tranformation logic
            void setup(Model &model, const std::vector<string>& attrs, ValueFunctor func){
                this->model = &model;
                for(auto& attr : attrs){ attribute_names.push_back(attr); }
                this->func = func;
                this->enabled = true;

                // register listener to transform realtime attribute changes
                this->model->attributeChangeEvent.addListener([this](Model::AttrChangeArgs& args){
                    // only works when enabled
                    if(!this->enabled)
                        return;

                    // see if we "have" the changed attribute
                    bool have_attribute = false;
                    for(auto& attr : this->attribute_names){
                        if(attr == args.attr){
                            have_attribute = true;
                            break;
                        }
                    }

                    // only works if we are specified for the changed attribute
                    if(!have_attribute)
                        return;

                    // perform "transformation"
                    this->func(args.value);
                }, this);

                // transform current value
                for(auto& attr : this->attribute_names)
                    this->func(model.get(attr));
            }

            void destroy(){
                if(this->model){
                    this->model->attributeChangeEvent.removeListeners(this);
                    this->model = NULL;
                }
            }

            //! (re-)enable this transformer
            void start(){ this->enabled=true; }
            //! (temporarily) disable this transformer
            void stop(){ this->enabled=false; }

        private:
            //! the model who's attribute this transformer works on
            Model* model;
            //! the name of the model's attributes that this transformer works on
            std::vector<string> attribute_names;
            //! the transformer "logic", provided at setup
            ValueFunctor func;
            //! the (toggle-able) enabled state of this transformer
            bool enabled;
    };

    //! transformer that actively (monitoring for realtime change) transforms an specified ofxCMS::Model instance's attribute through a lambda (provided by owner)
    class ModelTransformer {
        public:
            typedef FUNCTION<void(Model&)> Functor;

        public:
            ModelTransformer(Model &model, Functor func){
                this->model = &model;
                this->func = func;
                this->setup();
                this->start();
            }

            ~ModelTransformer(){ destroy(); }

            void destroy(){
                if(this->model){
                    this->model->changeEvent.removeListeners(this);
                    // this->model = NULL;
                }
            }

            //! register this transformer on the specified model for all the attributes in attrs, using the given func lambda for tranformation logic
            void setup(){
                // register listener to transform realtime attribute changes
                this->model->changeEvent.addListener([this](Model& m){
                    // only works when enabled
                    if(this->enabled)
                        this->func(m);
                }, this);

                this->func(*this->model);
            }

            //! (re-)enable this transformer
            void start(){ this->enabled=true; }
            //! (temporarily) disable this transformer
            void stop(){ this->enabled=false; }

        private:
            //! the model who's attribute this transformer works on
            Model* model;
            //! the transformer "logic", provided at setup
            Functor func;
            //! the (toggle-able) enabled state of this transformer
            bool enabled;
    };

    class ExtendedModel : public Model {
    public: // types
        typedef FUNCTION<void(const string&)> AttributeTransformFunctor;
        typedef FUNCTION<void(Model&)> ModelTransformFunctor;

    public:
        ExtendedModel() : valueTransformerRefs(nullptr), modelTransformerRefs(nullptr){}

        //! convenience method that converts the float value into a string
        // ExtendedModel* set(const string &attr, float value, bool notify = true);
        //! convenience method that converts the int value into a string
        // ExtendedModel* set(const string &attr, int value, bool notify = true);

        //! registers an attribute transformer
        shared_ptr<ValueTransformer> transform(const string& attr, AttributeTransformFunctor func, bool active=true){
            return transform(attr, func, NULL, active);
        }

        //! register the same transformer on various attributes
        shared_ptr<ValueTransformer> transform(const std::vector<string> &attrs, AttributeTransformFunctor func, bool active=true){
            return transform(attrs, func, NULL, active);
        }

        shared_ptr<ValueTransformer> transform(const string& attr, AttributeTransformFunctor func, void* owner, bool active=true){
            std::vector<string> attrs;
            attrs.push_back(attr);
            return transform(attrs, func, owner, active);
        }

        shared_ptr<ValueTransformer> transform(const std::vector<string> &attrs, AttributeTransformFunctor func, void* owner, bool active=true){
            auto transformerRef = std::make_shared<ValueTransformer>();
            transformerRef->setup(*this, attrs, func);

            // only save to our internal vector if active; otherwise let the shared_ptr expire
            if(active){
                if(!valueTransformerRefs)
                    valueTransformerRefs = std::make_shared<std::map<void*, std::vector<shared_ptr<ValueTransformer>>>>();
                (*valueTransformerRefs)[owner].push_back(transformerRef);
            }

            // give the transformer to the caller to they could use the start/stop functionality if they want
            return transformerRef;
        }

        std::vector<shared_ptr<ValueTransformer>> stopTransform(void* owner){
            std::vector<shared_ptr<ValueTransformer>> transformerRefs;

            if(valueTransformerRefs){
                transformerRefs = (*valueTransformerRefs)[owner];

                (*valueTransformerRefs)[owner].clear();

                for(auto transformerRef : transformerRefs)
                    transformerRef->stop();
            }

            return transformerRefs;
        }

    public: // model transformer methods

        std::shared_ptr<ModelTransformer> transform(ModelTransformFunctor func, bool active){
            return transform(func, NULL, active);
        }

        shared_ptr<ModelTransformer> transform(ModelTransformFunctor func, void* owner = NULL, bool active=true){
            auto transformerRef = std::make_shared<ModelTransformer>(*this, func);

            // only save to our internal vector if active; otherwise let the shared_ptr expire
            if(active){
                // lazy-initialize transformers container to save memory
                if(!modelTransformerRefs)
                    modelTransformerRefs = std::make_shared<std::map<void*, std::vector<shared_ptr<ModelTransformer>>>>();

                (*modelTransformerRefs)[owner].push_back(transformerRef);
            } else {
                transformerRef->stop();
            }

            // give the transformer to the caller to they could use the start/stop functionality if they want
            return transformerRef;
        }

        std::vector<shared_ptr<ModelTransformer>> stopModelTransforms(void* owner){
            std::vector<shared_ptr<ModelTransformer>> transformerRefs;

            if(modelTransformerRefs){
                transformerRefs = (*modelTransformerRefs)[owner];

                (*modelTransformerRefs)[owner].clear();

                for(auto transformerRef : transformerRefs)
                    transformerRef->stop();
            }

            return transformerRefs;
        }

    private:
        //! internal list of active value transformers, grouped by owner see .transform methods
        std::shared_ptr<std::map<void*, std::vector<shared_ptr<ValueTransformer>>>> valueTransformerRefs;
        std::shared_ptr<std::map<void*, std::vector<shared_ptr<ModelTransformer>>>> modelTransformerRefs;
    };
}
