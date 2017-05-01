#pragma once

#include "BaseCollection.h"

namespace ofxCMS {
    template<class ModelClass>
    class CollectionSync : public BaseCollection<ModelClass> {
        public: // methods

            CollectionSync() : target(NULL), source(nullptr), active(false){}
            ~CollectionSync(){ destroy(); }

            void setup(BaseCollection<ModelClass> *target, shared_ptr<BaseCollection<ModelClass>> source, bool active=true);
            void destroy();

            shared_ptr<BaseCollection<ModelClass>> getSource() const { return source; }

        private: // methods

            void sync();

        private: // attributes

            BaseCollection<ModelClass> *target;
            shared_ptr<BaseCollection<ModelClass>> source;
            bool active;
    };
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::setup(BaseCollection<ModelClass> *target, shared_ptr<BaseCollection<ModelClass>> source, bool active){
    destroy();

    this->target = target;
    this->source = source;
    this->active = active;
    this->sync();

    if(source && active){
        source->modelAddedEvent.addListener([this](ModelClass& model){
            this->sync();
        }, this);

        source->modelRemoveEvent.addListener([this](ModelClass& model){
            if(this->target->has(model.cid()))
                this->target->removeByCid(model.cid());
        }, this);
    }
}

template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::destroy(){
    if(source){
        source->modelAddedEvent.removeListeners(this);
        source->modelRemoveEvent.removeListeners(this);
        source = NULL;
    }
}

//! adds all models from source that are not in target to target
template<class ModelClass>
void ofxCMS::CollectionSync<ModelClass>::sync(){
    source->each([this](shared_ptr<ModelClass> modelRef){
        if(!this->target->has(modelRef)){
            this->target->add(modelRef);
        }
    });
}
