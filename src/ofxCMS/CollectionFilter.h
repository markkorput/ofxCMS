#pragma once

#include "BaseCollection.h"
#include "ofxLambdaEvent/function.h"

namespace ofxCMS {
    template<class ModelClass>
    class CollectionFilter : public BaseCollection<ModelClass> {

        public: // types & constants
            typedef FUNCTION<bool(ModelClass&)> Functor;

        public:
            CollectionFilter() : collection(NULL){};
            ~CollectionFilter(){ destroy(); }
            void setup(BaseCollection<ModelClass>* collection, const string& attr, const string& value, bool accept=true);
            void setup(BaseCollection<ModelClass>* collection, Functor func);
            void destroy();

        private:
            BaseCollection<ModelClass>* collection;
            Functor func;
    };
}

template<class ModelClass>
void ofxCMS::CollectionFilter<ModelClass>::setup(BaseCollection<ModelClass>* collection, const string& attr, const string& value, bool accept){
    // create function that checks a model
    auto func = [attr, value, accept](ModelClass& model) -> bool {
        bool match = model.get(attr) == value;
        return accept ? match : !match;
    };

    setup(collection, func);
}

template<class ModelClass>
void ofxCMS::CollectionFilter<ModelClass>::setup(BaseCollection<ModelClass>* collection, Functor func){
    destroy();
    this->collection = collection;
    this->func = func;

    // apply check to all currently added models
    collection->each([this](shared_ptr<ModelClass> model){
        if(!this->func(*model.get())){
            this->collection->removeByCid(model->cid());
        }
    });

    // apply to newly added models
    collection->beforeAdd.addListener(this->func, this);

    // apply to already added models when they are updated
    collection->modelChangeEvent.addListener([this](ModelClass& model){
        if(!this->func(model)){
            this->collection->removeByCid(model.cid());
        }
    }, this);
}

template<class ModelClass>
void ofxCMS::CollectionFilter<ModelClass>::destroy(){
    if(collection){
        collection->beforeAdd.removeListeners(this);
        collection->modelChangeEvent.removeListeners(this);
        collection = NULL;
    }
}
