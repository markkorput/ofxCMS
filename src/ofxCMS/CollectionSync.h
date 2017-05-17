#pragma once

#include "ModelCollection.h"

namespace ofxCMS {
    template<class ModelClass>
    class CollectionSync : public ModelCollection<ModelClass> {
        public: // methods

            CollectionSync() : target(NULL), source(nullptr){}
            ~CollectionSync(){ destroy(); }

            void setup(ModelCollection<ModelClass> *target, shared_ptr<ModelCollection<ModelClass>> source);
            void destroy();

            shared_ptr<ModelCollection<ModelClass>> getSource() const { return source; }

        private: // attributes

            ModelCollection<ModelClass> *target;
            shared_ptr<ModelCollection<ModelClass>> source;
    };
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::setup(ModelCollection<ModelClass> *target, shared_ptr<ModelCollection<ModelClass>> source){
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
        if(!this->target->has(model.cid())){
            this->target->add(this->source->findByCid(model.cid())); // need to convert ref var to shared_ptr
        }
    }, this);

    // actively montor for models removed from source
    source->removeEvent.addListener([this](ModelClass& model){
        if(this->target->has(model.cid()))
            this->target->removeByCid(model.cid());
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
