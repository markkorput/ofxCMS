#pragma once

#include "ObjectCollectionBase.h"

namespace ofxCMS {
    class ObjectTransformerBase {
    public:
        // ~ObjectTransformerBase(){ this->destroy(); }
        // virtual void destroy(){}
        virtual void* getSource() = 0;
    };

    template<class SourceType, class TargetType>
    class ObjectTransformer : public ObjectTransformerBase {
        public:
            typedef FUNCTION<shared_ptr<TargetType>(SourceType&)> TransformFunctor;

        public: // methods

            ObjectTransformer() : sourceCollection(NULL), targetCollection(NULL){}
            ~ObjectTransformer(){ ofLog() << "~ObjectTransformer"; destroy(); }

            void setup(
                ObjectCollectionBase<SourceType> &source,
                ObjectCollectionBase<TargetType> &target,
                TransformFunctor func);

            virtual void destroy();

            void* getSource() override { return (void*)sourceCollection; }

        private: // attributes

            ObjectCollectionBase<SourceType> *sourceCollection;
            ObjectCollectionBase<TargetType> *targetCollection;
            TransformFunctor func;
            std::map<SourceType*, TargetType*> transformLinks;
    };
}

template<class SourceType, class TargetType>
void ofxCMS::ObjectTransformer<SourceType, TargetType>::setup(
    ObjectCollectionBase<SourceType> &source,
    ObjectCollectionBase<TargetType> &target,
    TransformFunctor func){

    this->sourceCollection = &source;
    this->targetCollection = &target;
    this->func = func;

    // transform all instances currently in source
    this->sourceCollection->each([this](shared_ptr<SourceType> sourceRef){
        // generate tranformed instance
        auto targetRef = this->func(*sourceRef.get());
        this->targetCollection->add(targetRef);
        // record a "link-record" between the source and target instances
        this->transformLinks[sourceRef.get()] = targetRef.get();
    });

    // actively transform new instances appearing in source
    this->sourceCollection->addEvent.addListener([&](SourceType& sourceInstance){
        // generate tranformed instance
        auto targetRef = this->func(sourceInstance);
        this->targetCollection->add(targetRef);
        // record a "link-record" between the source and target instances
        this->transformLinks[&sourceInstance] = targetRef.get();
    }, this);

    // actively remove generated instances when instances are removed from source
    this->sourceCollection->removeEvent.addListener([this](SourceType& sourceInstance){
        // find "link-record" for removed source instance
        auto it = this->transformLinks.find(&sourceInstance);

        if(it == this->transformLinks.end()){
            // ofLogWarning() << "Could not find transformed match for removed source instance";
            return;
        }

        // remove the generated target instance
        this->targetCollection->remove(it->second);

        // remove the link
        this->transformLinks.erase(it);
    }, this);
}

template<class SourceType, class TargetType>
void ofxCMS::ObjectTransformer<SourceType, TargetType>::destroy(){
    if(this->sourceCollection){
        this->sourceCollection->addEvent.removeListeners(this);
        this->sourceCollection->removeEvent.removeListeners(this);
    }

    this->sourceCollection = NULL;
    this->targetCollection = NULL;
}
