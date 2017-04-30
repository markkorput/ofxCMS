// OF & addons
#include "ofxUnitTests.h"
// local
#include "ofxCMS.h"

#define TEST_START(x) {ofLog()<<"CASE: "<<#x;
#define TEST_END }

using namespace ofxCMS;

class ofApp: public ofxUnitTestsApp{

    void run(){
        // create collection
        shared_ptr<ofxCMS::Collection<ofxCMS::Model>> collectionRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();

        collectionRef->modelAddedEvent.addListener([](ofxCMS::Model& model){
            model.set("foo", "barr52");
        }, this);

        // create first model
        test_eq(collectionRef->count(), 0, "");
        shared_ptr<ofxCMS::Model> modelRef = collectionRef->create();
        test_eq(collectionRef->count(), 1, "");
        test_eq(modelRef->get("foo"), "barr52", "");

        collectionRef->modelAddedEvent.removeListeners(this);

        // default first id
        test_eq(modelRef->cid(), 1, "");
        // get non existing attribute
        test_eq(modelRef->get("name"), "", "");
        // get non existing attribute with default value
        test_eq(modelRef->get("name", "John Doe"), "John Doe", "");

        // changing attribute should trigger callbacks
        modelRef->attributeChangedEvent.addListener([](ofxCMS::AttrChangeArgs& args) -> void {
            args.model->set(args.attr, args.model->get(args.attr) + " (Model Callback OK)", false /* dony notify */);
        }, this);

        collectionRef->modelChangedEvent += [](ofxCMS::AttrChangeArgs& args) -> void {
            args.model->set(args.attr, args.model->get(args.attr) + " (Collection Callback OK)", false /* dony notify */);
        };
        // set name and trigger callback(s)
        modelRef->set("name", "Brian Fury");
        test_eq(modelRef->get("name"), "Brian Fury (Model Callback OK) (Collection Callback OK)", "");


        // add an existing external model without cid; gets assign a cid
        {
            ofxCMS::Model* m = new ofxCMS::Model();
            collectionRef->add(m);
            test_eq(collectionRef->count(), 2, "");
            test_eq(m->cid(), 2, "");
        }

        // add an existing external model without cid; gets assign a cid
        {
            ofxCMS::Model* m = new ofxCMS::Model();
            m->setCid(8);
            collectionRef->add(m);
            test_eq(collectionRef->count(), 3, "");
            test_eq(m->cid(), 8, "");
        }

        // add another model; gets the next cid (following the previous model's cid)
        {
            shared_ptr<ofxCMS::Model> m = collectionRef->create();
            test_eq(collectionRef->count(), 4, "");
            test_eq(m->cid(), 9, "");
        }

        // find and remove
        auto m = collectionRef->find(8);
        ofLog() << "TODO: test shared_ptr ref count";
        auto m2 = collectionRef->remove(m);
        test_eq(m, m2, "");
        ofLog() << "TODO: test shared_ptr ref count";
        test_eq(collectionRef->count(), 3, "");
        auto m3 = collectionRef->remove(3);
        test_eq((m3 == nullptr), true, "");
        test_eq(collectionRef->count(), 3, "");
        m3 = collectionRef->remove(2);
        test_eq((m3 == nullptr), false, "");
        test_eq(collectionRef->count(), 2, "");
        test_eq(m3->cid(), 9, "");
        ofLog() << "TODO: test shared_ptr ref count";

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
