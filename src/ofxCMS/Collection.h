#pragma once

#include "BaseCollection.h"
#include "CollectionLimit.h"
#include "CollectionSync.h"
#include "CollectionFilter.h"

namespace ofxCMS {
    template<class ModelClass>
    class Collection : public BaseCollection<ModelClass> {

    public: // methods
        Collection(){}

        void limit(unsigned int amount);
        void setFifo(bool newFifo){ collectionLimit.setFifo(newFifo); }

        void sync(shared_ptr<Collection<ModelClass>> other, bool active=true);
        void stopSync(shared_ptr<Collection<ModelClass>> other);

        void filter(const string& attr, const string& value, bool active=true);

    private: // attributes
        CollectionLimit<ModelClass> collectionLimit;
        std::vector<shared_ptr<CollectionSync<ModelClass>>> collectionSyncs;
        std::vector<shared_ptr<CollectionFilter<ModelClass>>> collectionFilters;
    };
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::limit(unsigned int amount){
    collectionLimit.setup(this, amount);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::sync(shared_ptr<Collection<ModelClass>> other, bool active){
    auto sync = make_shared<CollectionSync<ModelClass>>();
    sync->setup(this, other, active);
    ofLogWarning() << "implement active by not saving shared_ptr";
    collectionSyncs.push_back(sync);
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::stopSync(shared_ptr<Collection<ModelClass>> other){
    for(auto it = collectionSyncs.begin(); it != collectionSyncs.end(); it++){
        if(it->getSource() == other){
            collectionSyncs.erase(it);
            return;
        }
    }

    ofLogWarning() << "Could not source to stop syncing from";
}

template<class ModelClass>
void ofxCMS::Collection<ModelClass>::filter(const string& attr, const string& value, bool active){
    auto filter = make_shared<CollectionFilter<ModelClass>>();
    filter->setup(this, attr, value);
    //
    // // if active; save filter so it doesn't auto-destruct (since it's a shared_ptr)
    // if(active)
    //     collectionFilters.push_back(filter);
}
