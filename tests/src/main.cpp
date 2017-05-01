// OF & addons
#include "ofxUnitTests.h"
// local
#include "ofxCMS.h"

#define TEST_START(x) {ofLog()<<"CASE: "<<#x;
#define TEST_END }

using namespace ofxCMS;

class ofApp: public ofxUnitTestsApp{

    template<typename CollectionClass>
    shared_ptr<ofxCMS::Model> runCollection(){
        // create collection
        auto collectionRef = make_shared<CollectionClass>();

        TEST_START(add)
            collectionRef->modelAddedEvent.addListener([](ofxCMS::Model& model){
                model.set("foo", "barr52");
            }, this);

            // create first model
            test_eq(collectionRef->size(), 0, "");
            auto modelRef = collectionRef->create();

            test_eq(modelRef.use_count(), 2, "");

            collectionRef->modelAddedEvent.removeListeners(this);

            test_eq(collectionRef->size(), 1, "");
            test_eq(modelRef->get("foo"), "barr52", "");

            // default first id
            test_eq(modelRef->cid(), 1, "");
            // get non existing attribute
            test_eq(modelRef->get("name"), "", "");
            // get non existing attribute with default value
            test_eq(modelRef->get("name", "John Doe"), "John Doe", "");
        TEST_END

        TEST_START(attribute change callbacks)
            auto modelRef = collectionRef->at(0);

            // changing attribute should trigger callbacks
            modelRef->attributeChangeEvent.addListener([](ofxCMS::Model::AttrChangeArgs& args) -> void {
                args.model->set(args.attr, args.model->get(args.attr) + " (Model Callback OK)", false /* dony notify */);
            }, this);

            collectionRef->attributeChangeEvent.addListener([](ofxCMS::BaseCollection<ofxCMS::Model>::AttrChangeArgs& args) -> void {
                args.modelRef->set(args.attr, args.modelRef->get(args.attr) + " (Collection Callback OK)", false /* dony notify */);
            }, this);

            // set name and trigger callback(s)
            modelRef->set("name", "Brian Fury");
            test_eq(modelRef->get("name"), "Brian Fury (Collection Callback OK) (Model Callback OK)", "");

            collectionRef->attributeChangeEvent.removeListeners(this);
            modelRef->attributeChangeEvent.removeListeners(this);
        TEST_END

        TEST_START(add existing model without cid)
            auto m = make_shared<ofxCMS::Model>();
            collectionRef->add(m);
            test_eq(collectionRef->size(), 2, "");
            test_eq(m->cid(), 2, "");
        TEST_END

        TEST_START(add existing model with cid)
            auto m = make_shared<ofxCMS::Model>();
            m->setCid(8);
            collectionRef->add(m);
            test_eq(collectionRef->size(), 3, "");
            test_eq(m->cid(), 8, "");
        TEST_END

        TEST_START(check next cid)
            shared_ptr<ofxCMS::Model> model = collectionRef->create();
            test_eq(collectionRef->size(), 4, "");
            test_eq(model->cid(), 9, "");
        TEST_END

        TEST_START(find and remove)
            // find
            auto model = collectionRef->find(8);
            test_eq(model.use_count(), 2, "");
            // remove by reference
            model = collectionRef->remove(model);
            test_eq(model.use_count(), 1, ""); // local reference is last reference
            test_eq(collectionRef->size(), 3, "");
        TEST_END

        TEST_START(remove with invalid index)
            auto model = collectionRef->remove(3);
            test_eq(model == nullptr, true, "");
            test_eq(collectionRef->size(), 3, "");
        TEST_END

        TEST_START(remove by index)
            auto model = collectionRef->remove(2);
            test_eq(collectionRef->size(), 2, "");
            test_eq(model->cid(), 9, "");
            test_eq(model.use_count(), 1, ""); // last reference
        TEST_END

        TEST_START(destroy)
            collectionRef->destroy();
            test_eq(collectionRef->size(), 0, "");
        TEST_END

        ofLog() << "TODO: BaseCollection::initialize and initializeEvent";
        ofLog() << "TODO: BaseCollection::previous/next";

        // return an instance
        return collectionRef->create();
    }

    void run(){
        auto modelRef = runCollection<ofxCMS::BaseCollection<ofxCMS::Model>>();

        // TEST_START(change attribute after collection was deallocated)
        //     modelRef->set("foo101", "bar202");
        // TEST_END

        // modelRef = runCollection<ofxCMS::Collection<ofxCMS::Model>>();
        //
        // TEST_START(change attribute after collection was deallocated)
        //     modelRef->set("foo303", "bar404");
        // TEST_END
    }
};


#include "ofAppNoWindow.h"
#include "ofAppRunner.h"

int main( ){
  ofInit();
  auto window = std::make_shared<ofAppNoWindow>();
  auto app = std::make_shared<ofApp>();
  ofRunApp(window, app);
  return ofRunMainLoop();
}
