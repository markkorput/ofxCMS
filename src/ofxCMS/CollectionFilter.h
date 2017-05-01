#pragma once

#include "BaseCollection.h"

#ifndef FUNCTION
    #ifdef OFXCMS_USE_TR1
    	#include <tr1/functional>
    	#define FUNCTION tr1::function
    #else
    	#include <functional>
    	#define FUNCTION std::function
    #endif
#endif

namespace ofxCMS {
    template<class ModelClass>
    class CollectionFilter : public BaseCollection<ModelClass> {

        public: // types & constants
            typedef FUNCTION<bool(ModelClass&)> Functor;

        public:
            CollectionFilter() : collection(NULL){};
            ~CollectionFilter(){ destroy(); }
            void setup(BaseCollection<ModelClass>* collection, const string& attr, const string& value, bool accept=true);
            void destroy();

        private:
            BaseCollection<ModelClass>* collection;
    };
}

template<class ModelClass>
void ofxCMS::CollectionFilter<ModelClass>::setup(BaseCollection<ModelClass>* collection, const string& attr, const string& value, bool accept){
    destroy();
    this->collection = collection;

    auto func = [attr, value, accept](ModelClass& model) -> bool {
        bool match = model.get(attr) == value;
        return accept ? match : !match;
    };

    // apply check to all currently added models
    collection->each([collection, &func](shared_ptr<ModelClass> model){
        if(!func(*model.get())){
            collection->removeByCid(model->cid());
        }
    });

    collection->beforeAdd.addListener(func, this);
}

template<class ModelClass>
void ofxCMS::CollectionFilter<ModelClass>::destroy(){
    if(collection){
        collection->beforeAdd.removeListeners(this);
        collection = NULL;
    }
}
