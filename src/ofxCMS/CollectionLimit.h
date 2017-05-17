#pragma once

#include "ObjectCollectionBase.h"

#define OFXCMS_NO_LIMIT 0

namespace ofxCMS {

    template<class ObjectType>
    class CollectionLimit {
        public: // methods

            CollectionLimit() : collection(NULL), mLimit(OFXCMS_NO_LIMIT), bFifo(false){}
            ~CollectionLimit(){ destroy(); }

            void setup(ObjectCollectionBase<ObjectType>* collection, unsigned int amount);
            void destroy();

            // bool limitReached(){ return mLimit != NO_LIMIT && collection->size() >= mLimit; }
            bool limitExceeded(){ return mLimit != OFXCMS_NO_LIMIT && collection->size() > mLimit; }
            void setFifo(bool fifo){ bFifo = fifo; }
            // bool getFifo(){ return bFifo; }

        private: // methods

            void enforce();

        private: // attributes

            ObjectCollectionBase<ObjectType>* collection;
            unsigned int mLimit;
            bool bFifo; // first-in-first-out
    };
}

template<class ObjectType>
void ofxCMS::CollectionLimit<ObjectType>::setup(ObjectCollectionBase<ObjectType>* collection, unsigned int amount){
    destroy();
    this->collection = collection;
    this->mLimit = amount;

    this->collection->addEvent.addListener([this](ObjectType& instance){
        this->enforce();
    }, this);

    this->enforce();
}

template<class ObjectType>
void ofxCMS::CollectionLimit<ObjectType>::destroy(){
    if(collection){
        collection->addEvent.removeListeners(this);
        collection = NULL;
    }
}

template<class ObjectType>
void ofxCMS::CollectionLimit<ObjectType>::enforce(){
    while(limitExceeded()){
        collection->removeByIndex(bFifo ? 0 : collection->size()-1);
    }
}
