// OF & addons
#include "ofVec3f.h"
#include "ofxUnitTests.h"
// local
#include "ofxCRUD.h"

#define TEST_START(x) {ofLog()<<"CASE: "<<#x;
#define TEST_END }

using namespace ofxCRUD;

class ImageNode {
public:
    ImageNode(){ status = "uninitialized"; }
    string status;
    string filename;
    ofVec3f position, scale;

    string getStatus(){ return status; }
};

class ofApp: public ofxUnitTestsApp{

    void run(){
        TEST_START(LambdaEvent)
            ofLog() << "TODO";
        TEST_END


        TEST_START(Parameter::getEvent)
            Parameter<int> number;

            // normal ofParameter behaviour
            number.set(123);
            test_eq(number.get(), 123, "");
            test_eq(number.get(), 123, "");

            // getEvent
            number.getEvent += [](Parameter<int>& param){
                // change param value before it respond to the next .get call
                param.set(456);
            };

            test_eq(number.get(), 456, "");
        TEST_END


        TEST_START(Manager)
            // Add/Define a resouce
            ofxCRUD::Manager manager;


            TEST_START(Manager::createResource)
                test_eq(manager.getResources().size(), 0, "");
                auto resRef = manager.createResource<ImageNode>([](Resource<ImageNode>& def){
                    def.setName("ImageNode");
                    // // def->addInstantiator([](){ return make_shared<ImageNode>(); }); // optional, default to this
                    def.addProperty("status",
                        [](ImageNode& node){ return node.status; },
                        [](ImageNode& node, const string& value){ return node.status = value; });
                    def.addProperty("position");
                    def.addProperty("scale");
                });

                test_eq(manager.getResources().size(), 1, "");
                test_eq(manager.getResources()[0]->getName(), "ImageNode", "");
            TEST_END

            // fetch resource by name
            auto resourceRef = manager.getResource("ImageNode");
            // generate a new instance of that type
            shared_ptr<ImageNode> firstImageNodeRef = static_pointer_cast<ImageNode>(resourceRef->createInstance());

            TEST_START(CREATE)
                // "/ofxCRUD/ImageNode/create/start"
                // "/ofxCRUD/ImageNode/create/status", 2
                // "/ofxCRUD/ImageNode/create/position", 2,4,3
                // "/ofxCRUD/ImageNode/create/scale", 1,1,1
                // "/ofxCRUD/ImageNode/create/end"
                test_eq(firstImageNodeRef == nullptr, false, ""); // not null
                test_eq(firstImageNodeRef->getStatus(), "uninitialized", "");
                test_eq(resourceRef->getInstanceCount(), 1, "");
                // perform update through OSC
                ofxOscMessage oscMsg;
                oscMsg.setAddress("/ofxCRUD/ImageNode/create/2");
                manager.process(oscMsg);
                test_eq(resourceRef->getInstanceCount(), 2, "");
            TEST_END

            TEST_START(UPDATE)
                // find existing instance; failure
                shared_ptr<void> voidRef = resourceRef->find(123);
                test_eq(voidRef == nullptr, true,  "");
                // find existing instance; success
                shared_ptr<ImageNode> imgRef = static_pointer_cast<ImageNode>(resourceRef->find(1)); // 1 is default first id
                test_eq(imgRef == nullptr, false,  "");
                test_eq(imgRef, firstImageNodeRef,  "");

                // perform update directly on resource definition
                resourceRef->update(1 /* id */, "status" /* property */, "update1");
                test_eq(imgRef->status, "update1", "");

                // perform update through OSC
                ofxOscMessage oscMsg;
                oscMsg.setAddress("/ofxCRUD/ImageNode/update/1/status");
                oscMsg.addStringArg("update2");
                manager.process(oscMsg);

                // check if the param's change was propagated to the node's attribute
                test_eq(imgRef->status, "update2", "");
            TEST_END

            TEST_START("READ")
                // read from resource definition
                string value = resourceRef->read(1, "status");
                test_eq(value, "update2", "");

                // request attribute through oscMessage
                ofxOscMessage oscMsg;
                oscMsg.setAddress("/ofxCRUD/ImageNode/read/1/status");

                string responseAddr = "";
                string responseValue = "";

                manager.responseMessageEvent += [&responseAddr, &responseValue](ofxOscMessage msg){
                    responseAddr = msg.getAddress();
                    responseValue = msg.getArgAsString(0);
                };

                manager.process(oscMsg);

                test_eq(responseAddr, "/ofxCRUD/ImageNode/update/1/status", "");
                test_eq(responseValue, "update2", "");
            TEST_END

            TEST_START("DELETE")
                // delete instance directly on resource definition
                // (first create a new instance to remove)
                test_eq(resourceRef->getInstanceCount(), 2, "");
                shared_ptr<ImageNode> img2Ref = static_pointer_cast<ImageNode>(resourceRef->createInstance());
                test_eq(resourceRef->getInstanceCount(), 3, "");
                resourceRef->deleteInstance(img2Ref);
                test_eq(resourceRef->getInstanceCount(), 2, "");

                // delete instance through OSC message
                ofxOscMessage oscMsg;
                oscMsg.setAddress("/ofxCRUD/ImageNode/delete/1/status");
                manager.process(oscMsg);
                test_eq(resourceRef->getInstanceCount(), 1, "");
            TEST_END
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
