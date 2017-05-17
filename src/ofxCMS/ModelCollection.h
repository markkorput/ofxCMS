// #pragma once
//
// #include "BaseCollection.h"
//
// namespace ofxCMS {
//
//     //! collection that manages instances of an object-type that inherits from ofxCMS::Model
//     template<class ModelClass>
//     class ModelCollection : public BaseCollection<ModelClass> {
//     public:
//
//         // used in attributeChangeEvent notifications
//         struct AttrChangeArgs {
//             shared_ptr<ModelClass> modelRef;
//             string attr;
//             string value;
//         };
//
//     public:
//
//         ModelCollection();
//
//         void initialize(vector<map<string, string>>& _data);
//
//             // on add
//             // modelRef->changeEvent.addListener([this](Model& m){
//             //     modelChangeEvent.notifyListeners(*(this->findByCid(m.cid()).get()));
//             // }, this);
//             //
//             // modelRef->attributeChangeEvent.addListener([this](ofxCMS::Model::AttrChangeArgs& args) -> void {
//             //     // turn regular pointer into a shared_ptr (Ref) by looking it up in our internal ref list
//             //     auto changedModelRef = this->findByCid(args.model->cid());
//             //
//             //     if(changedModelRef == nullptr){
//             //         ofLogWarning() << "go attribute change from unknown model";
//             //         return;
//             //     }
//             //
//             //     AttrChangeArgs collectionArgs;
//             //     collectionArgs.modelRef = changedModelRef;
//             //     collectionArgs.attr = args.attr;
//             //     collectionArgs.value = args.value;
//             //     this->attributeChangeEvent.notifyListeners(collectionArgs);
//             // }, this);
//
//
//             // on remove
//             // remove callbacks
//             // instanceRef->attributeChangeEvent.removeListeners(this);
//             // instanceRef->changeEvent.removeListeners(this);
//
//
//         shared_ptr<ModelClass> findByCid(CidType cid);
//         shared_ptr<ModelClass> findById(const string& id, bool create=false);
//         std::vector<shared_ptr<ModelClass>> findByIds(const vector<string> ids, bool create=false);
//
//     private:
//
//         int indexOfId(const string& _id);
//
//     public: // events
//         LambdaEvent<ModelClass> modelAddedEvent;
//         LambdaEvent<BaseCollection<ModelClass>> initializeEvent;
//         LambdaEvent<ModelClass> modelChangeEvent;
//         LambdaEvent<AttrChangeArgs> attributeChangeEvent;
//         LambdaEvent<ModelClass> modelRemoveEvent;
//     };
//
// }
//
//
// template <class ModelClass>
// ofxCMS::ModelCollection<ModelClass>::ModelCollection(){
//     modelAddedEvent.forward(BaseCollection<ModelClass>::addEvent);
//     modelRemoveEvent.forward(BaseCollection<ModelClass>::removeEvent);
// }
//
// template <class ModelClass>
// shared_ptr<ModelClass> ofxCMS::ModelCollection<ModelClass>::findById(const string& id, bool create){
//     int idx = indexOfId(id);
//
//     // found; return found instance
//     if(idx != OFXCMS_INVALID_INDEX)
//         return BaseCollection<ModelClass>::at(idx);
//     // id not found
//     if(!create)
//         return nullptr;
//
//     auto newInstanceRef = this->create();
//     newInstanceRef->set("id", id);
//     return newInstanceRef;
// }
//
//
// template <class ObjectType>
// void ofxCMS::BaseCollection<ObjectType>::initialize(vector<map<string, string>>& _data){
//     // remove all existing data
//     destroy();
//
//     // create and add models
//     for(int i=0; i<_data.size(); i++){
//         auto newItem = make_shared<ObjectType>();
//         newItem->set(_data[i]);
//         add(newItem);
//     }
//
//     // notify
//     initializeEvent.notifyListeners(*this);
// }
//
// template <class ModelClass>
// std::vector<shared_ptr<ModelClass>> ofxCMS::BaseCollection<ModelClass>::findByIds(const vector<string> ids, bool create){
//     std::vector<shared_ptr<ModelClass>> result;
//
//     for(auto& _id : ids){
//         auto foundRef = findById(_id, create);
//         if(foundRef)
//             result.push_back(foundRef);
//     }
//
//     return result;
// }
//
// template <class ObjectType>
// int ofxCMS::BaseCollection<ObjectType>::indexOfId(const string& _id){
//     int idx=0;
//
//     for(auto modelRef : modelRefs){
//         if(modelRef->getId() == _id)
//             return idx;
//         idx++;
//     }
//
//     return OFXCMS_INVALID_INDEX;
// }
