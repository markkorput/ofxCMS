#pragma once

#include "ofxLambdaEvent/LambdaEvent.h"
#include "ObjectCollectionBase.h"

namespace ofxCMS {
        //! ModelCollection is a "core-essential" base class for the main Collection class.
        template<class ModelClass>
        class ModelCollection : public ObjectCollectionBase<ModelClass> {

        public: // types & constants

            typedef ModelClass* CidType;

            // used in attributeChangeEvent notifications
            struct AttrChangeArgs {
                shared_ptr<ModelClass> modelRef;
                string attr;
                string value;
            };

        public: // methods

            ModelCollection();

            void initialize(vector<map<string, string>>& _data);
            shared_ptr<ModelClass> findByCid(CidType cid){ return this->find(cid); }
            shared_ptr<ModelClass> findById(const string& id, bool create=false);
            std::vector<shared_ptr<ModelClass>> findByIds(const vector<string> ids, bool create=false);
            shared_ptr<ModelClass> removeByCid(CidType cid, bool notify=true){ return this->remove(cid, notify); }

        private: // methods

            int indexOfCid(CidType cid){ this->indexOf(cid); }
            int indexOfId(const string& _id);

        public: // events

            LambdaEvent<ModelCollection<ModelClass>> initializeEvent;
            LambdaEvent<ModelClass> modelChangeEvent;
            LambdaEvent<AttrChangeArgs> attributeChangeEvent;
    };
}


template <class ModelClass>
ofxCMS::ModelCollection<ModelClass>::ModelCollection(){
    // for every model that gets added to our collection, we register change listeners
    // that trigger our changeEvent and attributeChangeEvent
    this->addEvent.addListener([this](ModelClass& newModel){
        newModel.changeEvent.addListener([this](ofxCMS::Model& m){
            //this->modelChangeEvent.notifyListeners(*((ModelClass*)&m));
            this->modelChangeEvent.notifyListeners(*this->find((ModelClass*)&m).get());
        }, this);

        newModel.attributeChangeEvent.addListener([this](ofxCMS::Model::AttrChangeArgs& args){
            // turn regular pointer into a shared_ptr (Ref) by looking it up in our internal ref list
            auto changedModelRef = this->find((ModelClass*)args.model);

            if(changedModelRef == nullptr){
                ofLogWarning() << "got attribute change from unknown model";
                return;
            }

            AttrChangeArgs collectionArgs;
            collectionArgs.modelRef = changedModelRef;
            collectionArgs.attr = args.attr;
            collectionArgs.value = args.value;
            this->attributeChangeEvent.notifyListeners(collectionArgs);
        }, this);
    }, this);

    this->removeEvent.addListener([this](ModelClass& removeInstance){
        removeInstance.attributeChangeEvent.removeListeners(this);
        removeInstance.changeEvent.removeListeners(this);
    }, this);

}

template <class ModelClass>
void ofxCMS::ModelCollection<ModelClass>::initialize(vector<map<string, string>>& _data){
    // remove all existing data
    this->destroy();

    // create and add models
    for(int i=0; i<_data.size(); i++){
        auto newModel = make_shared<ModelClass>();
        newModel->set(_data[i]);
        this->add(newModel);
    }

    // notify
    initializeEvent.notifyListeners(*this);
}

template <class ModelClass>
shared_ptr<ModelClass> ofxCMS::ModelCollection<ModelClass>::findById(const string& id, bool create){
    int idx = indexOfId(id);
    // found; return found instance
    if(idx != OFXCMS_INVALID_INDEX)
        return this->at(idx);
    // id not found
    if(!create)
        return nullptr;

    auto newInstanceRef = this->create();
    newInstanceRef->set("id", id);
    return newInstanceRef;
}

template <class ModelClass>
std::vector<shared_ptr<ModelClass>> ofxCMS::ModelCollection<ModelClass>::findByIds(const vector<string> ids, bool create){
    std::vector<shared_ptr<ModelClass>> result;

    for(auto& _id : ids){
        auto foundRef = findById(_id, create);
        if(foundRef)
            result.push_back(foundRef);
    }

    return result;
}

template <class ModelClass>
int ofxCMS::ModelCollection<ModelClass>::indexOfId(const string& _id){
    int idx=OFXCMS_INVALID_INDEX;

    this->lock([this, &idx, &_id](){
        for(int i=this->size()-1; i>=0; i--){
            if(this->at(i)->getId() == _id){
                idx=i;
                return;
            }
        }
    });

    return idx;
}
