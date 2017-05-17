#pragma once

#include "ObjectCollectionBase.h"
#include "ofxLambdaEvent/function.h"

namespace ofxCMS {
    template<class InstanceType>
    class ObjectFilter : public ObjectCollectionBase<InstanceType> {

        public: // types & constants
            typedef FUNCTION<bool(InstanceType&)> Functor;

        public:
            ObjectFilter() : collection(NULL){};
            ~ObjectFilter(){ destroy(); }
            void setup(ObjectCollectionBase<InstanceType>* collection, Functor func, bool accept=true);
            void destroy();

        private:
            ObjectCollectionBase<InstanceType>* collection;
            Functor func;
    };
}

template<class InstanceType>
void ofxCMS::ObjectFilter<InstanceType>::setup(ObjectCollectionBase<InstanceType>* collection, Functor func, bool accept){
    destroy();
    this->collection = collection;
    this->func = func;

    // apply check to all currently added models
    collection->each([this, accept](shared_ptr<InstanceType> model){
        if(this->func(*model.get()) != accept){
            this->collection->remove(model);
        }
    });

    // apply to newly added models
    if(accept){
        collection->beforeAdd.addListener(this->func, this);
    } else {
        collection->beforeAdd.addListener([this](InstanceType& instance) -> bool {
            return !this->func(instance);
        }, this);
    }
}

template<class InstanceType>
void ofxCMS::ObjectFilter<InstanceType>::destroy(){
    if(collection){
        collection->beforeAdd.removeListeners(this);
        collection = NULL;
    }
}
