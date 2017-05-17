#pragma once

#include "ObjectCollectionBase.h"

namespace ofxCMS {
    template<class ObjectType>
    class CollectionSync : public ObjectCollectionBase<ObjectType> {
        public: // methods

            CollectionSync() : target(NULL), source(nullptr){}
            ~CollectionSync(){ destroy(); }

            void setup(ObjectCollectionBase<ObjectType> *target, shared_ptr<ObjectCollectionBase<ObjectType>> source);
            void destroy();

            shared_ptr<ObjectCollectionBase<ObjectType>> getSource() const { return source; }

        private: // attributes

            ObjectCollectionBase<ObjectType> *target;
            shared_ptr<ObjectCollectionBase<ObjectType>> source;
    };
}

template<class ObjectType>
void ofxCMS::CollectionSync<ObjectType>::setup(ObjectCollectionBase<ObjectType> *target, shared_ptr<ObjectCollectionBase<ObjectType>> source){
    destroy();

    this->target = target;
    this->source = source;

    if(!source){
        ofLogWarning() << "got nullptr source to sync from";
        return;
    }

    // add all instances currently in source
    source->each([this](shared_ptr<ObjectType> instanceRef){
        if(!this->target->has(instanceRef)){
            this->target->add(instanceRef);
        }
    });

    // actively monitor for new instances added to source
    source->addEvent.addListener([&](ObjectType& instance){
        if(!this->target->has(&instance)){
            this->target->add(this->source->find(&instance)); // need to convert ref var to shared_ptr
        }
    }, this);

    // actively montor for instances removed from source
    source->removeEvent.addListener([this](ObjectType& instance){
        if(this->target->has(&instance))
            this->target->remove(&instance);
    }, this);
}

template<class ObjectType>
void ofxCMS::CollectionSync<ObjectType>::destroy(){
    if(source){
        source->addEvent.removeListeners(this);
        source->removeEvent.removeListeners(this);
        source = NULL;
    }
}
