#pragma once

#include "BaseCollection.h"

namespace ofxCMS {
    template<class ModelClass>
    class CollectionSync : public BaseCollection<ModelClass> {
        public: // methods

            CollectionSync() : target(NULL), source(nullptr){}
            ~CollectionSync(){ destroy(); }

            void setup(BaseCollection<ModelClass> *target, shared_ptr<BaseCollection<ModelClass>> source);
            void destroy();

            shared_ptr<BaseCollection<ModelClass>> getSource() const { return source; }

        private: // attributes

            BaseCollection<ModelClass> *target;
            shared_ptr<BaseCollection<ModelClass>> source;
    };
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::setup(BaseCollection<ModelClass> *target, shared_ptr<BaseCollection<ModelClass>> source){
    destroy();

    this->target = target;
    this->source = source;

    if(!source){
        ofLogWarning() << "got nullptr source to sync from";
        return;
    }

    // add all models currently in source
    source->each([this](shared_ptr<ModelClass> modelRef){
        if(!this->target->has(modelRef)){
            this->target->add(modelRef);
        }
    });

    // actively monitor for new models added to source
    source->modelAddedEvent.addListener([&](ModelClass& model){
        if(!this->target->has(model.cid())){
            this->target->add(source->findByCid(model.cid())); // need to convert ref var to shared_ptr
        }
    }, this);

    // actively montor for models removed from source
    source->modelRemoveEvent.addListener([this](ModelClass& model){
        if(this->target->has(model.cid()))
            this->target->removeByCid(model.cid());
    }, this);
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::destroy(){
    if(source){
        source->modelAddedEvent.removeListeners(this);
        source->modelRemoveEvent.removeListeners(this);
        source = NULL;
    }
}
