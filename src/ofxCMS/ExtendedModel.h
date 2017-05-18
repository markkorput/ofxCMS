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

        void setup(Model &model, const string& attr, ValueFunctor func){
            this->model = &model;
            this->attr = attr;
            this->func = func;
            this->enabled = true;

            // register listener to transform realtime attribute changes
            this->model->attributeChangeEvent.addListener([this](Model::AttrChangeArgs& args){
                // only work on specified attribute and when enabled
                if(args.attr != this->attr || !this->enabled)
                    return;
                // perform "transformation"
                this->func(args.value);
            }, this);

            // transform current value
            this->func(model.get(this->attr));
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
        //! the name of the model's attribute that this transformer works on
        string attr;
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

    private:
        //! internal list of active value transformers, see .transform method
        std::vector<shared_ptr<ValueTransformer>> valueTransformerRefs;
    };
}
