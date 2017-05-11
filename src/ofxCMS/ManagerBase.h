#pragma once

#include "ofxLambdaEvent/function.h"

namespace ofxCMS {
    template<class CollectionClass>
    class ManagerBase {

        public: // types & constants

            typedef FUNCTION<void(void)> LockFunctor;
            typedef FUNCTION<void(shared_ptr<CollectionClass>, const string&)> CollectionRefFunc;

        public:
            ManagerBase() : lockCount(0){}

            //! \brief Gives a shared_ptr to a manager collection
            //! \param name identifies the desired collection
            //! \param create specifies if a collection with the given name should be created if no such collection exists yet
            shared_ptr<CollectionClass> get(const string& name, bool create=true);

            //! remove managed collection by name
            void remove(const string& name);

            //! Loop over each managed collection using the given lambda
            void each(CollectionRefFunc func);

            //! Gives the number of collections currently managed
            unsigned int size() const { return collectionRefs.size(); }

        private:
            //! Returns if the manager collectionRefs map is locked (being iterated over)
            bool isLocked() const { return lockCount > 0; }
            //! \brief Protects the internal state from getting corrupted when it's in use;
            // modifications like get and remove can still be called, but are queued and
            // executed after current operations are finished
            void lock(LockFunctor func);

        private:
            std::map<string, shared_ptr<CollectionClass>> collectionRefs;
            //! Hold the number of active locks; multiple simultanous (nested) is allowed; that' why we use a counter instead of a bool
            unsigned int lockCount;
    };
}

// IMPLEMENTATION OF ofxCMS::ManagerBase TEMPLATE CLASS

template<class CollectionClass>
shared_ptr<CollectionClass> ofxCMS::ManagerBase<CollectionClass>::get(const string& name, bool create){
    auto it = collectionRefs.find(name);

    // found!
    if(it != collectionRefs.end())
        return it->second;

    // not found, abort
    if(!create)
        return nullptr;

    // not found, create
    auto newCollection = make_shared<CollectionClass>();
    collectionRefs[name] = newCollection;
    return newCollection;
}

template <class CollectionClass>
void ofxCMS::ManagerBase<CollectionClass>::remove(const string& name){
    collectionRefs.erase(name);
}


template <class CollectionClass>
void ofxCMS::ManagerBase<CollectionClass>::each(CollectionRefFunc func){
    // lock first, to protect internal map while we iterate over it
    lock([&](){
        for(auto nameCollectionPair : this->collectionRefs){
            func(nameCollectionPair.second, nameCollectionPair.first);
        }
    });
}

template <class CollectionClass>
void ofxCMS::ManagerBase<CollectionClass>::lock(LockFunctor func){
    // lock (nested locks are allowed, that's why we're using a counter instead of a boolean)
    lockCount++;
    // execute operations that require lock
    func();
    // unlock
    lockCount--;

    // still locked (possible due to recursive lock); don't process queue yet
    if(isLocked())
        return;

    // after we're done iterating, we should process any items
    // accumulated in the vector modificaton queue
    ofLogWarning() << "ofxCMS::ManagerBase POST-lock queue processing not implemented yet";
    // for(auto modification : operationsQueue){
    //     if(modification->addRef){
    //         add(modification->addRef, modification->notify);
    //     } else {
    //         removeByCid(modification->removeCid, modification->notify);
    //     }
    // }

    // operationsQueue.clear();
}
