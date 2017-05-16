#pragma once

#include "ofxLambdaEvent/LambdaEvent.h"
#include "ofxLambdaEvent/Middleware.h"

#define OFXCMS_INVALID_INDEX (-1)
#define OFXCMS_INVALID_CID 0

// ObjectCollection is a "core-essential" base class for the main Collection class.
// ObjectCollection only implement the bare basics of creating, adding, update and removing items.

namespace ofxCMS {
    template<class ObjectType>
    class ObjectCollection{

        public: // types & constants

            typedef FUNCTION<void(void)> LockFunctor;
            typedef FUNCTION<void(shared_ptr<ObjectType>)> IterateRefFunc;

            class Modification {
                public:
                    shared_ptr<ObjectType> addRef;
                    ObjectType* removeCid;
                    bool notify;
                    Modification() : addRef(nullptr), removeCid(OFXCMS_INVALID_CID), notify(true){}
                    Modification(shared_ptr<ObjectType> ref, bool _notify=true) : addRef(ref), removeCid(OFXCMS_INVALID_CID), notify(_notify){}
                    Modification(ObjectType* cid, bool _notify=true) : addRef(nullptr), removeCid(cid), notify(_notify){}
            };

        public: // methods

            ObjectCollection() : vectorLockCount(0){}
            ~ObjectCollection(){ destroy(); }

            void destroy();

            // CRUD - Create
            shared_ptr<ObjectType> create();
            void add(shared_ptr<ObjectType> instanceRef, bool notify=true);

            // CRUD - Read
            // const vector<shared_ptr<ObjectType>> &instances(){ return instanceRefs; }
            shared_ptr<ObjectType> at(unsigned int idx);
            shared_ptr<ObjectType> find(ObjectType* cid);

            unsigned int size(){ return instanceRefs.size(); }
            bool has(shared_ptr<ObjectType> instanceRef){ return indexOf(instanceRef->get()) != OFXCMS_INVALID_INDEX; }
            bool has(ObjectType* cid){ return indexOf(cid) != OFXCMS_INVALID_INDEX; }

            int randomIndex(){ return size() == 0 ? OFXCMS_INVALID_INDEX : floor(ofRandom(size())); }
            shared_ptr<ObjectType> random(){ return at(randomIndex()); }
            shared_ptr<ObjectType> previous(shared_ptr<ObjectType> instanceRef, bool wrap=false);
            shared_ptr<ObjectType> next(shared_ptr<ObjectType> instanceRef, bool wrap=false);

            // CRUD - "update"
            void each(IterateRefFunc func);

            // CRUD - Delete
            shared_ptr<ObjectType> remove(shared_ptr<ObjectType> instanceRef, bool notify=true);
            shared_ptr<ObjectType> removeByCid(ObjectType* cid, bool notify=true);
            shared_ptr<ObjectType> removeByIndex(unsigned int index, bool notify=true);

        protected: // methods

            int indexOf(ObjectType* cid);
            bool isLocked() const { return vectorLockCount > 0; }
            void lock(LockFunctor func);

        public: // events

            Middleware<ObjectType> beforeAdd;
            LambdaEvent<ObjectType> addEvent;
            LambdaEvent<ObjectType> removeEvent;

        private: // attributes

            std::vector<shared_ptr<ObjectType>> instanceRefs;
            unsigned int vectorLockCount;
            std::vector<shared_ptr<Modification>> operationsQueue;
    };
}

template <class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::destroy(){
    if(isLocked()){
        ofLogWarning() << "ofxCMS::ObjectCollection destroyed while locked";
    }

    for(int i=instanceRefs.size()-1; i>=0; i--){
        removeByIndex(i);
    }
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::create(){
    // create instance with auto-incremented ID
    auto ref = make_shared<ObjectType>();
    // add to our collection and return
    add(ref);
    return ref;
}

template <class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::add(shared_ptr<ObjectType> instanceRef, bool notify){
    // vector being iterated over? schedule removal operation for when iteration is done
    if(isLocked()){
        operationsQueue.push_back(make_shared<Modification>(instanceRef, notify));
        return;
    }

    if(instanceRef == nullptr){
        // What the hell are we supposed to do with this??
        ofLogWarning() << "got nullptr instance to add to collection";
        return;
    }

    if(!beforeAdd(*instanceRef.get()))
        return;

    // add to our collection
    instanceRefs.push_back(instanceRef);

    // let's tell the world
    if(notify)
        ofNotifyEvent(addEvent, *(instanceRef.get()), this);

    // success!
    return;
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::previous(shared_ptr<ObjectType> instanceRef, bool wrap){
    int idx = indexOf(instanceRef.get());

    if(idx == OFXCMS_INVALID_INDEX)
        return nullptr;

    if(idx > 0)
        return at(idx-1);

    if(wrap)
        return at(size()-1);

    return nullptr;
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::next(shared_ptr<ObjectType> instanceRef, bool wrap){
    int idx = indexOf(instanceRef.get());

    if(idx == OFXCMS_INVALID_INDEX)
        return nullptr;

    if(idx < size()-1)
        return at(idx+1);

    if(wrap)
        return at(0);

    return nullptr;
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::at(unsigned int idx){
    if(idx < 0 || idx >= size()){
        ofLogWarning() << "invalid index";
        return nullptr;
    }

    return instanceRefs.at(idx);
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::find(ObjectType* cid){
    int idx = indexOf(cid);
    if(idx == OFXCMS_INVALID_INDEX)
        return nullptr;
    return at(idx);
}

template <class ObjectType>
int ofxCMS::ObjectCollection<ObjectType>::indexOf(ObjectType* cid){
    int idx=0;

    for(auto instanceRef : instanceRefs){
        if(instanceRef.get() == cid)
            return idx;
        idx++;
    }

    return OFXCMS_INVALID_INDEX;
}

template <class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::each(IterateRefFunc func){
    // when locked all add/remove operations to our internal instanceRefs vector
    // are intercepted and queue for execution until after the lock is lifter,
    // so we can safely iterate over the vector and callbacks are free to call
    // our add/remove methods without causing errors
    lock([&](){
        for(auto instanceRef : this->instanceRefs){
            func(instanceRef);
        }
    });
}

template <class ObjectType>
shared_ptr<ObjectType>  ofxCMS::ObjectCollection<ObjectType>::remove(shared_ptr<ObjectType> instanceRef, bool notify){
    if(instanceRef == nullptr){
        ofLogWarning() << "got NULL parameter";
        return nullptr;
    }

    // find index and remove by index
    return removeByCid(instanceRef.get());
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::removeByCid(ObjectType* cid, bool notify){
    // vector being iterated over? schedule removal operation for when iteration is done
    if(isLocked()){
        operationsQueue.push_back(make_shared<Modification>(cid, notify));
        return nullptr;
    }

    int idx = indexOf(cid);

    if(idx == OFXCMS_INVALID_INDEX){
        ofLogWarning() << "could not find instance to remove";
        return nullptr;
    }

    // find
    auto instanceRef = at(idx);

    // remove
    instanceRefs.erase(instanceRefs.begin() + idx);

    // notify
    if(notify)
        removeEvent.notifyListeners(*instanceRef.get());

    // return removed instance
    return instanceRef;
}

template <class ObjectType>
shared_ptr<ObjectType> ofxCMS::ObjectCollection<ObjectType>::removeByIndex(unsigned int index, bool notify){
    auto instanceRef = at(index);

    // check
    if(!instanceRef){
        ofLogWarning() << "couldn't find instance with index: " << index;
        return nullptr;
    }

    // invoke main remove routine
    return removeByCid(instanceRef.get(), notify);
}

template <class ObjectType>
void ofxCMS::ObjectCollection<ObjectType>::lock(LockFunctor func){
    vectorLockCount++;
    func();
    vectorLockCount--;

    // still (recursively) iterating over our vector? skip processing opereations queue
    if(isLocked())
        return;

    // after we're done iterating, we should process any items
    // accumulated in the vector modificaton queue
    for(auto modification : operationsQueue){
        if(modification->addRef){
            add(modification->addRef, modification->notify);
        } else {
            removeByCid(modification->removeCid, modification->notify);
        }
    }

    operationsQueue.clear();
}
