#pragma once

#include "ObjectCollectionBase.h"
#include "CollectionLimit.h"
#include "CollectionSync.h"

namespace ofxCMS {
    template<class ObjectType>
    class ObjectCollection : public ObjectCollectionBase<ObjectType> {

    public:

        void limit(unsigned int amount);
        void setFifo(bool newFifo){ collectionLimit.setFifo(newFifo); }

        void sync(shared_ptr<ObjectCollectionBase<ObjectType>> other, bool active=true);
        void stopSync(shared_ptr<ObjectCollectionBase<ObjectType>> other);

    private:
        CollectionLimit<ObjectType> collectionLimit;
        std::vector<shared_ptr<CollectionSync<ObjectType>>> collectionSyncs;
    };
}

template<class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::limit(unsigned int amount){
    collectionLimit.setup(this, amount);
}

template<class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::sync(shared_ptr<ObjectCollectionBase<ObjectType>> other, bool active){
    auto sync = make_shared<CollectionSync<ObjectType>>();
    sync->setup(this, other);

    // if active safe pointer so it doesn't auto-destruct
    if(active)
        collectionSyncs.push_back(sync);
}

template<class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::stopSync(shared_ptr<ObjectCollectionBase<ObjectType>> other){
    for(auto it = collectionSyncs.begin(); it != collectionSyncs.end(); it++){
        if(it->getSource() == other){
            collectionSyncs.erase(it);
            return;
        }
    }

    ofLogWarning() << "Could not source to stop syncing from";
}
