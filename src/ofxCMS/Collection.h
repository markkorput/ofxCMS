#pragma once

#include "BaseCollection.h"
#include "CollectionLimit.h"
#include "CollectionSync.h"

namespace ofxCMS {
    template<class ModelClass>
    class Collection : public BaseCollection<ModelClass> {

    public: // methods
        Collection(){}

        void limit(unsigned int amount);
        void setFifo(bool newFifo){ collectionLimit.setFifo(newFifo); }

        void sync(shared_ptr<Collection<ModelClass>> other, bool active=true);

    private: // attributes
        CollectionLimit<ModelClass> collectionLimit;
        CollectionSync<ModelClass> collectionSync;
    };
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::limit(unsigned int amount){
    collectionLimit.setup(this, amount);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::sync(shared_ptr<Collection<ModelClass>> other, bool active){
    collectionSync.setup(this, other.get(), active);
}
