#pragma once

#include "BaseCollection.h"

namespace ofxCMS {

    template<class ModelClass>
    class CollectionLimit {
        public: // types a contants

            const static unsigned int NO_LIMIT = 0;

        public: // methods

            CollectionLimit() : collection(NULL), mLimit(NO_LIMIT), bFifo(false){}
            ~CollectionLimit(){ destroy(); }

            void setup(BaseCollection<ModelClass>* collection, unsigned int amount);
            void destroy();

            // bool limitReached(){ return mLimit != NO_LIMIT && collection->size() >= mLimit; }
            bool limitExceeded(){ return mLimit != NO_LIMIT && collection->size() > mLimit; }
            void setFifo(bool fifo){ bFifo = fifo; }
            // bool getFifo(){ return bFifo; }

        private: // methods

            void enforce();

        private: // attributes

            BaseCollection<ModelClass>* collection;
            unsigned int mLimit;
            bool bFifo; // first-in-first-out
    };
}

template<class ModelClass>
void ofxCMS::CollectionLimit<ModelClass>::setup(BaseCollection<ModelClass>* collection, unsigned int amount){
    destroy();
    this->collection = collection;
    this->mLimit = amount;

    this->collection->modelAddedEvent.addListener([this](ModelClass& model){
        this->enforce();
    }, this);

    this->enforce();
}

template<class ModelClass>
void ofxCMS::CollectionLimit<ModelClass>::destroy(){
    if(collection){
        collection->modelAddedEvent.removeListeners(this);
        collection = NULL;
    }
}

template<class ModelClass>
void ofxCMS::CollectionLimit<ModelClass>::enforce(){
    while(limitExceeded()){
        collection->removeByIndex(bFifo ? 0 : collection->size()-1);
    }
}
