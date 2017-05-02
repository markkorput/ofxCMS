// OF & addons
#include "ofxUnitTests.h"
// local
#include "ofxCMS.h"
#include "ofxCMS/lib/Middleware.h"

#define TEST_START(x) {ofLog()<<"CASE: "<<#x;
#define TEST_END }

using namespace ofxCMS;

class ofApp: public ofxUnitTestsApp{

    template<typename CollectionClass>
    shared_ptr<ofxCMS::Model> runCollection(){
        // create collection
        auto collectionRef = make_shared<CollectionClass>();
        unsigned int cid;

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

            cid = modelRef->cid();
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
            test_eq(m->cid(), cid+1, "");
        TEST_END

        TEST_START(add existing model with cid)
            auto m = make_shared<ofxCMS::Model>();
            m->setCid(cid+20);
            collectionRef->add(m);
            test_eq(collectionRef->size(), 3, "");
            test_eq(m->cid(), cid+20, "");
        TEST_END

        TEST_START(check next cid)
            shared_ptr<ofxCMS::Model> model = collectionRef->create();
            test_eq(collectionRef->size(), 4, "");
            test_eq(model->cid(), cid+21, "");
        TEST_END

        TEST_START(find and remove)
            // find
            auto model = collectionRef->at(3);
            test_eq(model.use_count(), 2, "");

            // remove by reference
            model = collectionRef->remove(model);
            test_eq(model.use_count(), 1, ""); // local reference is last reference
            test_eq(collectionRef->size(), 3, "");
        TEST_END

        TEST_START(remove with invalid index)
            auto model = collectionRef->removeByIndex(3);
            test_eq(model == nullptr, true, "");
            test_eq(collectionRef->size(), 3, "");
        TEST_END

        TEST_START(remove by index)
            auto model = collectionRef->removeByIndex(2);
            test_eq(collectionRef->size(), 2, "");
            test_eq(model->cid(), cid+20, "");
            test_eq(model.use_count(), 1, ""); // last reference
        TEST_END

        TEST_START(destroy)
            collectionRef->destroy();
            test_eq(collectionRef->size(), 0, "");
        TEST_END

        TEST_START(initialize and each)
            collectionRef->create();
            test_eq(collectionRef->size(), 1, "");

            std::vector<std::map<string, string>> data;
            std::map<string, string> m;
            m["number"] = "one";
            data.push_back(m);
            m["number"] = "two";
            data.push_back(m);

            collectionRef->initializeEvent.addListener([](ofxCMS::BaseCollection<ofxCMS::Model>& col){
                // TODO
                col.each([](shared_ptr<ofxCMS::Model> modelRef){
                    modelRef->set("number", "#"+modelRef->get("number"));
                });
            }, this);

            collectionRef->initialize(data);
            test_eq(collectionRef->size(), 2, "");
            test_eq(collectionRef->at(0)->get("number"), "#one", "");
            test_eq(collectionRef->at(1)->get("number"), "#two", "");
        TEST_END

        TEST_START(previous n next)
            test_eq(collectionRef->previous(collectionRef->at(1))->get("number"), "#one", "");
            test_eq(collectionRef->previous(collectionRef->at(0), true)->get("number"), "#two", "");
            test_eq(collectionRef->previous(collectionRef->at(0)) == nullptr, true, "");
            test_eq(collectionRef->next(collectionRef->at(0))->get("number"), "#two", "");
            test_eq(collectionRef->next(collectionRef->at(1), true)->get("number"), "#one", "");
            test_eq(collectionRef->next(collectionRef->at(1)) == nullptr, true, "");
        TEST_END

        // return an instance
        return collectionRef->create();
    }

    void run(){
        TEST_START(ofxLiquidEvent modified while invoked)
            ofxLiquidEvent<ofxCMS::Model> event;
            ofxCMS::Model model;

            event.addListener([&](ofxCMS::Model& m){
                model.set("before", ofToString(event.size()));
                // modify event (remove listener) while being invoked
                // (this callback gets called by the event itself)
                event.removeListeners(this);
                model.set("after", ofToString(event.size()));
            }, this);

            // one callback; the one we just registered
            test_eq(event.size(), 1, "");

            // invoke callback
            event.notifyListeners(model);
            test_eq(model.get("before"), "1", "");
            // still 1, the remove won't take effect until
            // the event is idle again
            test_eq(model.get("after"), "1", "");
            // the callback was removed immediately after the last listener finished running
            test_eq(event.size(), 0, "");
        TEST_END

        TEST_START(middleware aborts)
            Middleware<ofxCMS::Model> mid;

            mid.addListener([](ofxCMS::Model& m) -> bool {
                m.set("name", m.get("name") + " #1");
                return true;
            }, this);

            mid.addListener([](ofxCMS::Model& m) -> bool {
                m.set("name", m.get("name") + " #2");
                return false; // <-- aborts!
            }, this);

            mid.addListener([](ofxCMS::Model& m) -> bool {
                m.set("name", m.get("name") + " #3");
                return true;
            }, this);

            ofxCMS::Model m;
            test_eq(mid.notifyListeners(m), false, "");
            test_eq(m.get("name"), " #1 #2", "");
        TEST_END

        TEST_START(middleware continues)
            Middleware<ofxCMS::Model> mid;

            mid.addListener([](ofxCMS::Model& m) -> bool {
                m.set("name", m.get("name") + " #1");
                return true;
            }, this);

            mid.addListener([](ofxCMS::Model& m) -> bool {
                m.set("name", m.get("name") + " #2");
                return true;
            }, this);

            mid.addListener([](ofxCMS::Model& m) -> bool {
                m.set("name", m.get("name") + " #3");
                return true;
            }, this);

            ofxCMS::Model m;
            test_eq(mid.notifyListeners(m), true, "");
            test_eq(m.get("name"), " #1 #2 #3", "");
        TEST_END

        auto modelRef = runCollection<ofxCMS::BaseCollection<ofxCMS::Model>>();
        unsigned int cid;

        TEST_START(change attribute after collection was deallocated)
            modelRef->set("foo101", "bar202");
            test_eq(modelRef.use_count(), 1, "");
        TEST_END

        modelRef = runCollection<ofxCMS::Collection<ofxCMS::Model>>();

        TEST_START(change attribute after collection was deallocated)
            modelRef->set("foo303", "bar404");
            test_eq(modelRef.use_count(), 1, "");
        TEST_END

        TEST_START(limit)
            auto colRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // create five instance
            colRef->create();
            colRef->create();
            colRef->create();
            colRef->create();
            colRef->create();
            cid = colRef->at(0)->cid();

            string removed = "";
            colRef->modelRemoveEvent.addListener([&removed](ofxCMS::Model& model){
                removed += "#"+ofToString(model.cid());
            }, this);

            colRef->limit(3);
            test_eq(colRef->size(), 3, ""); // two models removed
            test_eq(removed, "#"+ofToString(cid+4)+"#"+ofToString(cid+3), ""); // remove callback invoked; last two models removed

            colRef->create();
            test_eq(colRef->size(), 3, ""); // nothing added (fifo is false by default)
            test_eq(colRef->at(2)->cid(), cid+2, "");

            colRef->setFifo(true);
            colRef->create();
            test_eq(colRef->size(), 3, ""); // nothing added (fifo is false by default)
            test_eq(colRef->at(2)->cid(), cid+6, "");

            colRef->modelRemoveEvent.removeListeners(this);
        TEST_END

        TEST_START(sync once)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            auto colRefB = make_shared<ofxCMS::Collection<ofxCMS::Model>>();

            // initialize B with one model
            colRefB->create();
            test_eq(colRefB->size(), 1, "");
            test_eq(colRefA->size(), 0, "");

            // sync operation transfers model to A
            colRefA->sync(colRefB, false /* sync once, don't monitor for changes */);
            test_eq(colRefB->size(), 1, "");
            test_eq(colRefA->size(), 1, "");
            test_eq(colRefA->at(0).get(), colRefB->at(0).get(), "");

            // sync is not active; A won't receive new models from B
            colRefB->create();
            test_eq(colRefB->size(), 2, "");
            test_eq(colRefA->size(), 1, "");
            test_eq(colRefA->at(0).get(), colRefB->at(0).get(), "");

            // sync is not active; A won't drop models along with B
            colRefB->removeByIndex(0);
            test_eq(colRefB->size(), 1, "");
            test_eq(colRefA->size(), 1, "");
            test_eq(colRefA->at(0).get() == colRefB->at(0).get(), false, "");
        TEST_END

        TEST_START(sync active)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            auto colRefB = make_shared<ofxCMS::Collection<ofxCMS::Model>>();

            // initialize B with one model
            colRefB->create();
            test_eq(colRefB->size(), 1, "");
            test_eq(colRefA->size(), 0, "");

            // sync operation transfers model to A
            colRefA->sync(colRefB);
            test_eq(colRefB->size(), 1, "");
            test_eq(colRefA->size(), 1, "");
            test_eq(colRefA->at(0).get(), colRefB->at(0).get(), "");

            // active sync; A receives new models from B
            colRefB->create();
            test_eq(colRefB->size(), 2, "");
            test_eq(colRefA->size(), 2, "");
            test_eq(colRefA->at(1).get(), colRefB->at(1).get(), "");

            // second sync source
            auto colRefC = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            colRefC->create();
            colRefC->create();
            test_eq(colRefC->size(), 2, "");
            colRefA->sync(colRefC);
            test_eq(colRefA->size(), 4, "");

            colRefC->create();
            test_eq(colRefA->size(), 5, "");

            // active sync; A drops models along with B
            colRefB->removeByIndex(0);
            colRefB->removeByIndex(0);
            test_eq(colRefB->size(), 0, "");
            test_eq(colRefA->size(), 3, "");

            colRefC->removeByIndex(0);
            colRefC->removeByIndex(0);
            test_eq(colRefC->size(), 1, "");
            test_eq(colRefA->size(), 1, "");
        TEST_END

        TEST_START(filter once on specific value)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // add three models with different age values
            auto modelRef = colRefA->create();
            modelRef->set("Age", "12");
            modelRef = colRefA->create();
            modelRef->set("Age", "25");
            modelRef = colRefA->create();
            modelRef->set("Age", "31");
            modelRef = colRefA->create();
            modelRef->set("Age", "46");
            // filter on specific age value
            colRefA->filter("Age", "31", false); // perform one-time filter
            test_eq(colRefA->size(), 1, "");
            test_eq(colRefA->at(0)->get("Age"), "31", "");
            // add new model
            colRefA->create();
            test_eq(colRefA->size(), 2, "");
            test_eq(colRefA->at(1)->get("Age"), "", "");
        TEST_END

        TEST_START(reject once on specific value)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // add three models with different age values
            auto modelRef = colRefA->create();
            modelRef->set("Age", "12");
            modelRef = colRefA->create();
            modelRef->set("Age", "25");
            modelRef = colRefA->create();
            modelRef->set("Age", "31");
            modelRef = colRefA->create();
            modelRef->set("Age", "46");
            // filter on specific age value
            colRefA->reject("Age", "31", false); // perform one-time filter
            test_eq(colRefA->size(), 3, "");
            test_eq(colRefA->at(0)->get("Age"), "12", "");
            // add new model
            modelRef = make_shared<ofxCMS::Model>();
            modelRef->set("Age", "31");
            colRefA->add(modelRef);
            test_eq(colRefA->size(), 4, "");
        TEST_END

        TEST_START(filter actively on specific value)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // add three models with different age values
            auto modelRef = colRefA->create();
            modelRef->set("Age", "12");
            modelRef = colRefA->create();
            modelRef->set("Age", "25");
            modelRef = colRefA->create();
            modelRef->set("Age", "31");
            modelRef = colRefA->create();
            modelRef->set("Age", "46");
            // filter on specific age value
            colRefA->filter("Age", "31"); // enable perform active filter
            test_eq(colRefA->size(), 1, "");
            test_eq(colRefA->at(0)->get("Age"), "31", "");
            // add new model
            modelRef = colRefA->create();
            test_eq(colRefA->size(), 1, "");
            modelRef->set("Age", "31");
            colRefA->add(modelRef);
            test_eq(colRefA->size(), 2, "");
        TEST_END

        TEST_START(reject actively on specific value)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // add three models with different age values
            auto modelRef = colRefA->create();
            modelRef->set("Age", "12");
            modelRef = colRefA->create();
            modelRef->set("Age", "25");
            modelRef = colRefA->create();
            modelRef->set("Age", "31");
            modelRef = colRefA->create();
            modelRef->set("Age", "46");
            // filter on specific age value
            colRefA->reject("Age", "31"); // enable perform active filter
            test_eq(colRefA->size(), 3, "");
            test_eq(colRefA->at(0)->get("Age"), "12", "");
            // add new model
            modelRef = colRefA->create(); // adds
            test_eq(colRefA->size(), 4, "");
            modelRef->set("Age", "32"); // nothing happens
            test_eq(colRefA->size(), 4, "");
            modelRef->set("Age", "31"); // gets rejected
            test_eq(colRefA->size(), 3, "");
        TEST_END

        TEST_START(filter actively using custom lambda)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // add three models with different age values
            auto modelRef = colRefA->create();
            modelRef->set("Age", "12");
            modelRef = colRefA->create();
            modelRef->set("Age", "25");
            modelRef = colRefA->create();
            modelRef->set("Age", "31");
            modelRef = colRefA->create();
            modelRef->set("Age", "46");
            // filter on specific age value
            colRefA->filter([](ofxCMS::Model& model) -> bool {
                return ofToInt(model.get("Age")) >= 21;
            });
            test_eq(colRefA->size(), 3, "");
            test_eq(colRefA->at(0)->get("Age"), "25", "");
            // add new model
            modelRef = colRefA->create();
            test_eq(colRefA->size(), 3, ""); // not added
            modelRef->set("Age", "20");
            colRefA->add(modelRef);
            test_eq(colRefA->size(), 3, ""); // not added
            modelRef->set("Age", "21");
            colRefA->add(modelRef);
            test_eq(colRefA->size(), 4, ""); // now it's added
            modelRef->set("Age", "19");
            test_eq(colRefA->size(), 3, ""); // now it's removed again
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
