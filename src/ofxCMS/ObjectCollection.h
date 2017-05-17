#pragma once

#include "ObjectCollectionBase.h"
#include "CollectionLimit.h"
#include "CollectionSync.h"
#include "ObjectFilter.h"
#include "ObjectTransformer.h"

namespace ofxCMS {
    template<class ObjectType>
    class ObjectCollection : public ObjectCollectionBase<ObjectType> {

    public: // types & constants
        typedef FUNCTION<bool(ObjectType&)> FilterFunctor;

    public:

        void limit(unsigned int amount);
        void setFifo(bool newFifo){ collectionLimit.setFifo(newFifo); }

        void sync(shared_ptr<ObjectCollectionBase<ObjectType>> other, bool active=true);
        void stopSync(shared_ptr<ObjectCollectionBase<ObjectType>> other);

        void filter(FilterFunctor func, bool active=true);
        void reject(FilterFunctor func, bool active=true);

        template<class SourceType>
        void transform(ObjectCollectionBase<SourceType> &sourceCollection, FUNCTION<shared_ptr<ObjectType>(SourceType&)> func/*, bool active=true*/){
            // create
            auto transformerRef = make_shared<ObjectTransformer<SourceType, ObjectType>>();
            // configure
            transformerRef->setup(sourceCollection, *this, func);
            // store
            objectTransformers.push_back(transformerRef);
        }

        template<class SourceType>
        void stopTransform(ObjectCollectionBase<SourceType> &sourceCollection){
            // search
            for(auto it = objectTransformers.begin(); it != objectTransformers.end(); it++){
                // find
                if((*it)->getSource() == (void*)&sourceCollection){
                    // remove/destroy
                    objectTransformers.erase(it);
                    return;
                }
            }

            ofLogWarning() << "Could not find source collection to stop transforming from";
        }

    private:
        CollectionLimit<ObjectType> collectionLimit;
        std::vector<shared_ptr<CollectionSync<ObjectType>>> collectionSyncs;
        std::vector<shared_ptr<ObjectFilter<ObjectType>>> collectionFilters;
        std::vector<shared_ptr<ObjectTransformerBase>> objectTransformers;
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

    ofLogWarning() << "Could not find source to stop syncing from";
}

template<class InstanceType>
void ofxCMS::ObjectCollection<InstanceType>::filter(FilterFunctor func, bool active){
    auto filter = make_shared<ObjectFilter<InstanceType>>();
    filter->setup(this, func);

    // if active; save filter so it doesn't auto-destruct (since it's a shared_ptr)
    if(active)
        collectionFilters.push_back(filter);
}

template<class InstanceType>
void ofxCMS::ObjectCollection<InstanceType>::reject(FilterFunctor func, bool active){
    auto filter = make_shared<ObjectFilter<InstanceType>>();
    filter->setup(this, func, false /* reject instead of accept */);

    // if active; save filter so it doesn't auto-destruct (since it's a shared_ptr)
    if(active)
        collectionFilters.push_back(filter);
}
