// OF & addons
#include "ofxUnitTests.h"
// local
#include "ofxCMS.h"

#define TEST_START(x) {ofLog()<<"CASE: "<<#x;
#define TEST_END }

using namespace ofxCMS;

class ofApp: public ofxUnitTestsApp{

    void run(){
        // create collection and first instance
        shared_ptr<ofxCMS::Collection<ofxCMS::Model>> collectionRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
        shared_ptr<ofxCMS::Model> modelRef = collectionRef->create();
        // default first id
        test_eq(modelRef->id(), "1", "");
        // get non existing attribute
        test_eq(modelRef->get("name"), "", "");
        // get non existing attribute with default value
        test_eq(modelRef->get("name", "John Doe"), "John Doe", "");


        // changing attribute should trigger callbacks
        modelRef->attributeChangedEvent.addListener([](ofxCMS::AttrChangeArgs& args) -> void {
            args.model->set(args.attr, args.value + " (Model Callback OK)", false /* dony notify */);
        }, this);
        collectionRef->modelChangedEvent += [](ofxCMS::AttrChangeArgs& args) -> void {
            args.model->set(args.attr, args.value + " (Collection Callback OK)", false /* dony notify */);
        };
        // set name and trigger callback(s)
        modelRef->set("name", "Brian Fury");
        test_eq(modelRef->get("name"), "Brian Fury (Model Callback OK) (Collection callback OK)", "");


        TEST_START(Adding existing model might need modify nextId)
            ofLog() << "TODO";
        TEST_END
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
