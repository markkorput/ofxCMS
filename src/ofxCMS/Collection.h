#pragma once

#include "BaseCollection.h"
#include "CollectionLimit.h"

namespace ofxCMS {
    template<class ModelClass>
    class Collection : public BaseCollection<ModelClass> {

    public: // methods
        Collection(){}

        void limit(unsigned int amount);
        void setFifo(bool newFifo){ collectionLimit.setFifo(newFifo); }

    private: // attributes
        CollectionLimit<ModelClass> collectionLimit;

    };
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::limit(unsigned int amount){
    collectionLimit.setup(this, amount);
}
