#pragma once

#include "BaseCollection.h"

namespace ofxCMS {
    template<class ModelClass>
    class CollectionSync : public BaseCollection<ModelClass> {
        public: // methods

            CollectionSync() : target(NULL), source(NULL), active(false){}
            ~CollectionSync(){ destroy(); }

            void setup(BaseCollection<ModelClass>* target, BaseCollection<ModelClass>* source, bool active=true);
            void destroy();

        private: // methods

            void sync();

        private: // attributes

            BaseCollection<ModelClass> *target, *source;
            bool active;
    };
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::setup(BaseCollection<ModelClass>* target, BaseCollection<ModelClass>* source, bool active){
    this->target = target;
    this->source = source;
    this->active = active;
    this->sync();
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::destroy(){
    if(source){
        source = NULL;
    }
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::sync(){
    source->each([this](shared_ptr<ModelClass> modelRef){
        ofLog() << "sync: " << modelRef->cid();
        if(!this->target->has(modelRef)){
            ofLog() << "adding!";
            this->target->add(modelRef);
        }
    });
}
