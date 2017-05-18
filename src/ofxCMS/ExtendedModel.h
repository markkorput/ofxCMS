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

    class ExtendedModel : public Model {
    public: // types
        typedef FUNCTION<void(const string&)> AttributeTransformFunctor;

    public:
        //! convenience method that converts the float value into a string
        // ExtendedModel* set(const string &attr, float value, bool notify = true);
        //! convenience method that converts the int value into a string
        // ExtendedModel* set(const string &attr, int value, bool notify = true);

        //! registers an attribute transformer
        shared_ptr<ValueTransformer> transform(const string& attr, AttributeTransformFunctor func, bool active=true){
            auto transformerRef = make_shared<ValueTransformer>();
            transformerRef->setup(*this, attr, func);

            // only save to our internal vector if active; otherwise let the shared_ptr expire
            if(active)
                valueTransformerRefs.push_back(transformerRef);

            // give the transformer to the caller to they could use the start/stop functionality if they want
            return transformerRef;
        }

        //! convenience method to register the same transformer on various attributes
        shared_ptr<ValueTransformer> transform(const std::vector<string> &attrs, AttributeTransformFunctor func, bool active=true){
            auto transformerRef = make_shared<ValueTransformer>();
            transformerRef->setup(*this, attrs, func);

            // only save to our internal vector if active; otherwise let the shared_ptr expire
            if(active)
                valueTransformerRefs.push_back(transformerRef);

            // give the transformer to the caller to they could use the start/stop functionality if they want
            return transformerRef;
        }

    private:
        //! internal list of active value transformers, see .transform method
        std::vector<shared_ptr<ValueTransformer>> valueTransformerRefs;
    };
}
