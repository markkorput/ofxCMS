#pragma once

#include "ObjectCollection.h"

namespace ofxCMS {
    template<class ModelClass>
    class CollectionSync : public ObjectCollection<ModelClass> {
        public: // methods

            CollectionSync() : target(NULL), source(nullptr){}
            ~CollectionSync(){ destroy(); }

            void setup(ObjectCollection<ModelClass> *target, shared_ptr<ObjectCollection<ModelClass>> source);
            void destroy();

            shared_ptr<ObjectCollection<ModelClass>> getSource() const { return source; }

        private: // attributes

            ObjectCollection<ModelClass> *target;
            shared_ptr<ObjectCollection<ModelClass>> source;
    };
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::setup(ObjectCollection<ModelClass> *target, shared_ptr<ObjectCollection<ModelClass>> source){
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
    source->addEvent.addListener([&](ModelClass& model){
        if(!this->target->has(&model)){
            this->target->add(this->source->find(&model)); // need to convert ref var to shared_ptr
        }
    }, this);

    // actively montor for models removed from source
    source->removeEvent.addListener([this](ModelClass& model){
        if(this->target->has(&model))
            this->target->remove(&model);
    }, this);
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::destroy(){
    if(source){
        source->addEvent.removeListeners(this);
        source->removeEvent.removeListeners(this);
        source = NULL;
    }
}
