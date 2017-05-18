#include "ofxUnitTests.h"
#include "ofxCMS.h"
#include "ofxCMS/ObjectCollectionBase.h"
#include "ofxCMS/ObjectCollection.h"

#define TEST_START(x) {ofLog()<<"CASE: "<<#x;
#define TEST_END }

// initialize singleton instance
CMSMAN_INIT

typedef void* CidType;

class ofApp: public ofxUnitTestsApp{

    template<typename ModelType>
    void testModel(){
        TEST_START(each)
            ModelType model;
            std::vector<string> result;
            model.set("name", "John");
            model.set("age", "32");

            test_eq(ofJoinString(result, ","), "", "");
            model.each([&result](const string& attr, const string& val){
                result.push_back(attr+"="+val);
            });

            test_eq(ofJoinString(result, ","), "age=32,name=John", "");
        TEST_END

        TEST_START(each lock; making modifications while iterating over attributes)
            ModelType model;

            model.set("name", "John");
            model.set("age", "32");
            test_eq(model.size(), 2, "");
            test_eq(model.get("name_copy"), "", "");
            test_eq(model.get("age_copy"), "", "");

            std::vector<string> result;
            test_eq(ofJoinString(result, ","), "", "");

            model.each([&model, &result](const string& attr, const string& val){
                // add copy attribute
                model.set(attr+"_copy", val);
                model.set(attr, val+"_updated");
                // get number of attributes (the above new attribute should not be added yet)
                result.push_back(attr+"="+model.get(attr)+"(size:"+ofToString(model.size())+")");
            });

            // during the iterations, the model didn't change
            test_eq(ofJoinString(result, ","), "age=32(size:2),name=John(size:2)", "");
            // immediately after the iterations finished, all changes were effected
            test_eq(model.size(), 4, "");
            test_eq(model.get("name"), "John_updated", "");
            test_eq(model.get("age"), "32_updated", "");
            test_eq(model.get("name_copy"), "John", "");
            test_eq(model.get("age_copy"), "32", "");
        TEST_END
    }

    template<typename ModelType>
    void testExtendedModel(){
        TEST_START(actively transform specific attribute)
            ModelType model;
            model.set("age", "10");

            float result;

            auto transformerRef = model.transform("age", [&result](const string& value){
                result = ofToFloat(value) * 100.0f;
            });

            // .transform already processed the existing value
            test_eq(result, 1000.0f, "");
            // change value
            model.set("age", "25");
            // .transform registers change listener and automatically processes updated value
            test_eq(result, 2500.0f, "");

            // stop transformer
            transformerRef->stop();
            // change value
            model.set("age", "1");
            // didn't transform
            test_eq(result, 2500.0f, "");

            // start transformer again
            transformerRef->start();
            // change value
            model.set("age", "2");
            // DID transform
            test_eq(result, 200.0f, "");
        TEST_END

    }

    template<typename InstanceType>
    void testObjectCollectionBase(){
        auto collectionRef = make_shared<ofxCMS::ObjectCollectionBase<InstanceType>>();

        TEST_START(create)
            int count=0;

            collectionRef->addEvent.addListener([&count](InstanceType& model){
                count++;
            }, this);

            test_eq(collectionRef->size(), 0, "");
            test_eq(count, 0, "");

            // create first model
            auto instanceRef = collectionRef->create();
            test_eq(instanceRef.use_count(), 2, "");

            test_eq(collectionRef->size(), 1, "");
            test_eq(count, 1, "");

            collectionRef->addEvent.removeListeners(this);
            instanceRef = collectionRef->create();

            test_eq(collectionRef->size(), 2, "");
            test_eq(count, 1, "");
        TEST_END

        TEST_START(add)
            int startCount = collectionRef->size();
            auto m = make_shared<InstanceType>();
            collectionRef->add(m);
            test_eq(collectionRef->size(), startCount+1, "");
        TEST_END

        TEST_START(find and remove)
            int curCount = collectionRef->size();

            // find
            auto instanceRef = collectionRef->at(curCount-1);
            test_eq(instanceRef.use_count(), 2, "");

            // remove by reference
            collectionRef->remove(instanceRef);
            test_eq(instanceRef.use_count(), 1, ""); // local reference is last reference
            test_eq(collectionRef->size(), curCount-1, "");
        TEST_END

        TEST_START(remove with invalid index)
            int curCount = collectionRef->size();
            auto instanceRef = collectionRef->removeByIndex(collectionRef->size()+1);
            test_eq(instanceRef == nullptr, true, "");
            test_eq(collectionRef->size(), curCount, "");
        TEST_END

        TEST_START(remove by index)
            int curCount = collectionRef->size();
            auto instanceRef = collectionRef->at(curCount-1);
            test_eq(instanceRef.use_count(), 2, "");
            instanceRef = collectionRef->removeByIndex(curCount-1);
            test_eq(collectionRef->size(), curCount-1, "");
            test_eq(instanceRef.use_count(), 1, ""); // last reference
        TEST_END

        TEST_START(remove by pointer)
            ofLogWarning() << "TODO";
        TEST_END

        TEST_START(destroy)
            collectionRef->destroy();
            test_eq(collectionRef->size(), 0, "");
        TEST_END

        TEST_START(each)
            int curCount = collectionRef->size();
            collectionRef->create();
            test_eq(collectionRef->size(), curCount+1, "");

            int eachCount=0;

            collectionRef->each([&eachCount](shared_ptr<InstanceType> modelRef){
                eachCount++;
            });

            test_eq(eachCount, curCount+1, "");
        TEST_END

        TEST_START(add while iterating)
            ofLogWarning() << "TODO";
        TEST_END

        TEST_START(remove while iterating)
            ofLogWarning() << "TODO";
        TEST_END

        TEST_START(previous)
            ofLogWarning() << "TODO";
        TEST_END

        TEST_START(next)
            ofLogWarning() << "TODO";
        TEST_END

        TEST_START(random)
            ofLogWarning() << "TODO";
        TEST_END
    }

    template<typename InstanceType>
    void testObjectCollection(){

        TEST_START(limit)
            auto colRef = make_shared<ofxCMS::ObjectCollection<InstanceType>>();
            // create five instance
            colRef->create();
            colRef->create();
            colRef->create();
            colRef->create();
            colRef->create();

            string removed = "";

            colRef->removeEvent.addListener([&removed](InstanceType& model){
                removed += "#"+ofToString(&model);
            }, this);

            string expected = "#"+ofToString(colRef->at(4).get())+"#"+ofToString(colRef->at(3).get());
            colRef->limit(3);
            test_eq(colRef->size(), 3, ""); // two models removed
            test_eq(removed, expected, ""); // remove callback invoked; last two models removed

            InstanceType* cid = colRef->at(2).get();
            colRef->create();
            test_eq(colRef->size(), 3, ""); // nothing added (fifo is false by default)
            test_eq(colRef->at(2).get(), cid, "");

            colRef->setFifo(true);
            auto newModelRef = colRef->create();
            test_eq(colRef->size(), 3, ""); // nothing added (fifo is false by default)
            test_eq(colRef->at(2).get(), newModelRef.get(), "");

            colRef->removeEvent.removeListeners(this);

            ofLogWarning() << "TODO: also feature one-time (non-active) limit";
        TEST_END

        TEST_START(sync once)
            auto colRefA = make_shared<ofxCMS::ObjectCollection<InstanceType>>();
            auto colRefB = make_shared<ofxCMS::ObjectCollection<InstanceType>>();

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
            auto colRefA = make_shared<ofxCMS::ObjectCollection<InstanceType>>();
            auto colRefB = make_shared<ofxCMS::ObjectCollection<InstanceType>>();

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
            auto colRefC = make_shared<ofxCMS::ObjectCollection<InstanceType>>();
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

        TEST_START(filter actively using custom lambda)
            ofxCMS::ObjectCollection<InstanceType> col;
            col.create();
            col.create();
            col.create();
            test_eq(col.size(), 3, "");

            int filterCounter=0;

            col.filter([&filterCounter](InstanceType& instance) -> bool {
                // accept every other instance
                bool accept = ((filterCounter & 1) == 0);
                filterCounter++;
                return accept;
            });

            test_eq(filterCounter, 3, "");
            test_eq(col.size(), 2, "");

            col.create();
            test_eq(col.size(), 2, "");
            col.create();
            test_eq(col.size(), 3, "");
            col.create();
            test_eq(col.size(), 3, "");
            col.create();
            test_eq(col.size(), 4, "");
            test_eq(filterCounter, 7, "");
        TEST_END

        TEST_START(reject actively using custom lambda)
            ofxCMS::ObjectCollection<InstanceType> col;
            col.create();
            col.create();
            col.create();
            test_eq(col.size(), 3, "");

            int filterCounter=0;

            col.reject([&filterCounter](InstanceType& instance) -> bool {
                // accept every other instance
                bool accept = ((filterCounter & 1) == 0);
                filterCounter++;
                return accept;
            });

            test_eq(filterCounter, 3, "");
            test_eq(col.size(), 1, "");

            col.create();
            test_eq(col.size(), 2, "");
            col.create();
            test_eq(col.size(), 2, "");
            col.create();
            test_eq(col.size(), 3, "");
            col.create();
            test_eq(col.size(), 3, "");
            test_eq(filterCounter, 7, "");
        TEST_END

        TEST_START(combine filter sync and limit)
            ofLogWarning() << "TODO";
        TEST_END

        TEST_START(transform)
            class TransformedClass {
            public:
                string pointerString;
            };

            ofxCMS::ObjectCollection<TransformedClass> targetCol;
            ofxCMS::ObjectCollection<InstanceType> sourceCol;

            sourceCol.create();
            sourceCol.create();
            test_eq(sourceCol.size(), 2, "");
            test_eq(targetCol.size(), 0, "");

            targetCol.template transform<InstanceType>(sourceCol, [](InstanceType& source) -> shared_ptr<TransformedClass>{
                auto instanceRef = make_shared<TransformedClass>();
                instanceRef->pointerString = ofToString(&source);
                return instanceRef;
            });

            test_eq(targetCol.size(), 2, "");
            test_eq(targetCol.at(0)->pointerString, ofToString(sourceCol.at(0).get()), "");
            test_eq(targetCol.at(1)->pointerString, ofToString(sourceCol.at(1).get()), "");

            sourceCol.create();
            test_eq(targetCol.size(), 3, "");
            test_eq(targetCol.at(0)->pointerString, ofToString(sourceCol.at(0).get()), "");
            test_eq(targetCol.at(1)->pointerString, ofToString(sourceCol.at(1).get()), "");
            test_eq(targetCol.at(2)->pointerString, ofToString(sourceCol.at(2).get()), "");

            sourceCol.removeByIndex(1);
            test_eq(targetCol.size(), 2, "");
            test_eq(targetCol.at(0)->pointerString, ofToString(sourceCol.at(0).get()), "");
            test_eq(targetCol.at(1)->pointerString, ofToString(sourceCol.at(1).get()), "");

            targetCol.stopTransform(sourceCol);
            sourceCol.removeByIndex(0);
            test_eq(sourceCol.size(), 1, "");
            test_eq(targetCol.size(), 2, "");
            sourceCol.create();
            sourceCol.create();
            test_eq(sourceCol.size(), 3, "");
            test_eq(targetCol.size(), 2, "");
        TEST_END
    }

    template<typename CollectionClass>
    void testModelCollection(){
        // dummy is just so we can use auto in the second line; we don't know the model type!
        CollectionClass dummy;
        auto orphanModel = dummy.create();

        {
            auto collectionRef = make_shared<CollectionClass>();
            CidType cid;

            TEST_START(add)
                collectionRef->addEvent.addListener([](ofxCMS::Model& model){
                    model.set("foo", "barr52");
                }, this);

                // create first model
                test_eq(collectionRef->size(), 0, "");
                auto modelRef = collectionRef->create();

                test_eq(modelRef.use_count(), 2, "");

                collectionRef->addEvent.removeListeners(this);

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

                collectionRef->attributeChangeEvent.addListener([](ofxCMS::ModelCollection<ofxCMS::Model>::AttrChangeArgs& args) -> void {
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
                test_eq(m->cid(), m.get(), "");
            TEST_END

            TEST_START(find and remove)
                int curCount = collectionRef->size();
                // find
                auto model = collectionRef->at(curCount-1);
                test_eq(model.use_count(), 2, "");

                // remove by reference
                model = collectionRef->remove(model);
                test_eq(model.use_count(), 1, ""); // local reference is last reference
                test_eq(collectionRef->size(), curCount-1, "");
            TEST_END

            TEST_START(remove with invalid index)
                int curCount = collectionRef->size();
                auto model = collectionRef->removeByIndex(collectionRef->size()+1);
                test_eq(model == nullptr, true, "");
                test_eq(collectionRef->size(), curCount, "");
            TEST_END

            TEST_START(remove by index)
                int curCount = collectionRef->size();
                auto model = collectionRef->removeByIndex(collectionRef->size()-1);
                test_eq(collectionRef->size(), curCount-1, "");
                // test_eq(model->cid(), cid+20, "");
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

                collectionRef->initializeEvent.addListener([](ofxCMS::ModelCollection<ofxCMS::Model>& col){
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

            TEST_START(model copy)
                auto refA = collectionRef->create();
                auto refB = collectionRef->create();
                refA->set("id", "1");
                refA->set("_id", "_1");
                refA->set("firstname", "john");
                refA->set("lastname", "doe");
                refB->set("id", "2");
                refB->set("_id", "_2");
                test_eq(refB->get("firstname"), "", "");
                test_eq(refB->get("lastname"), "", "");
                refB->copy(refA); // copy, but keep id and _id properties
                test_eq(refB->get("id"), "2", "");
                test_eq(refB->get("_id"), "_2", "");
                test_eq(refB->get("firstname"), "john", "");
                test_eq(refB->get("lastname"), "doe", "");
                refA->set("firstname", "jane");
                refB->copy(refA, true); // copy including id and _id properties
                test_eq(refB->get("id"), "1", "");
                test_eq(refB->get("_id"), "_1", "");
                test_eq(refB->get("firstname"), "jane", "");
                test_eq(refB->get("lastname"), "doe", "");
            TEST_END

            TEST_START(model each)
                ofLogWarning() << "TODO";
            TEST_END

            TEST_START(findById)
                CollectionClass col;
                test_eq(col.size(), 0, "");
                auto modelRef = col.findById("foo");
                test_eq(modelRef == nullptr, true, "");
                test_eq(col.size(), 0, "");
                modelRef = col.findById("foo", true /* create if not found */);
                test_eq(modelRef == nullptr, false, "");
                test_eq(col.size(), 1, "");
                test_eq(col.findById("foo") == modelRef, true, "");
            TEST_END

            orphanModel = collectionRef->create();
        }

        orphanModel->set("some_attr","to_check_if_all_collection_listeners_are_unregistered");
        // properly orhpaned?
        test_eq(orphanModel.use_count(), 1, "");
    }

    void testCollectionAddons(){
        CidType cid;

        TEST_START(limit)
            auto colRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // create five instance
            colRef->create();
            colRef->create();
            colRef->create();
            colRef->create();
            colRef->create();

            string removed = "";

            colRef->removeEvent.addListener([&removed](ofxCMS::Model& model){
                removed += "#"+ofToString(model.cid());
            }, this);

            string expected = "#"+ofToString(colRef->at(4)->cid())+"#"+ofToString(colRef->at(3)->cid());
            colRef->limit(3);
            test_eq(colRef->size(), 3, ""); // two models removed
            test_eq(removed, expected, ""); // remove callback invoked; last two models removed

            cid = colRef->at(2)->cid();
            colRef->create();
            test_eq(colRef->size(), 3, ""); // nothing added (fifo is false by default)
            test_eq(colRef->at(2)->cid(), cid, "");

            colRef->setFifo(true);
            auto newModelRef = colRef->create();
            test_eq(colRef->size(), 3, ""); // nothing added (fifo is false by default)
            test_eq(colRef->at(2)->cid(), newModelRef->cid(), "");

            colRef->removeEvent.removeListeners(this);

            ofLogWarning() << "TODO: also feature one-time (non-active) limit";
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
            modelRef->set("Age", "36");
            test_eq(colRefA->size(), 3, ""); // not automatically re-added again, once its gone, need to re-add explicitly
        TEST_END

        TEST_START(combine filter sync and limit)
            auto colRefA = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            auto colRefB = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            // start with five models in B
            auto modelRef = colRefB->create();
            modelRef->set("value", "10");
            modelRef = colRefB->create();
            modelRef->set("value", "20");
            modelRef = colRefB->create();
            modelRef->set("value", "30");
            modelRef = colRefB->create();
            modelRef->set("value", "40");
            modelRef = colRefB->create();
            modelRef->set("value", "50");
            // A only takes tkes the models with value >= 30 from B
            colRefA->filter([](ofxCMS::Model& model){
                return ofToInt(model.get("value", "0")) >= 30;
            });
            colRefA->sync(colRefB);
            test_eq(colRefA->size(), 3, "");
            test_eq(colRefB->size(), 5, "");
            // add model to B
            modelRef = colRefB->create();
            test_eq(colRefA->size(), 3, "");
            test_eq(colRefB->size(), 6, "");
            // set its value to 60
            modelRef->set("value", "60");
            ofLogWarning() << "should this be 4?";
            test_eq(colRefA->size(), 3, ""); // not adopted, already rejected
            test_eq(colRefB->size(), 6, "");
            // apply limit
            colRefA->limit(2);
            test_eq(colRefA->size(), 2, "");
            colRefA->create();
            test_eq(colRefA->size(), 2, "");
            // add new model to B which -because of limit- doesn't get added to A
            modelRef = make_shared<ofxCMS::Model>();
            modelRef->set("value", "80");
            colRefB->add(modelRef);
            test_eq(colRefA->size(), 2, "");
            test_eq(colRefA->at(0)->get("value"), "30", "");
            test_eq(colRefA->at(1)->get("value"), "40", "");
            test_eq(colRefB->size(), 7, "");
            // change A to fifo; now it does get new models from B
            colRefA->setFifo(true);
            modelRef = make_shared<ofxCMS::Model>();
            modelRef->set("value", "99");
            colRefB->add(modelRef);
            test_eq(colRefA->size(), 2, "");
            test_eq(colRefA->at(0)->get("value"), "40", "");
            test_eq(colRefA->at(1)->get("value"), "99", "");
            test_eq(colRefB->size(), 8, "");
        TEST_END

        TEST_START(read json array collection)
            auto colRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            colRef->loadJsonFromFile("test.json");
            test_eq(colRef->size(), 3, "");
            test_eq(colRef->at(0)->get("number"), "one", "");
            test_eq(colRef->at(1)->get("number"), "two", "");
            test_eq(colRef->at(1)->getId(), "#2", "");
            test_eq(colRef->at(2)->get("number"), "three", "");
        TEST_END

        TEST_START(read json hash collection)
            auto colRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            colRef->loadJsonFromFile("test_with_keys.json");
            test_eq(colRef->size(), 3, "");
            test_eq(colRef->at(0)->get("name"), "the first", "");
            test_eq(colRef->at(0)->getId(), "id1", "");
            test_eq(colRef->at(1)->get("name"), "the second", "");
            test_eq(colRef->at(1)->getId(), "id2", "");
            test_eq(colRef->at(2)->get("name"), "the 3rd", "");
            test_eq(colRef->at(2)->getId(), "id3", "");
        TEST_END

        TEST_START(load and reload json with number values)
            auto colRef = make_shared<ofxCMS::Collection<ofxCMS::Model>>();
            colRef->loadJsonFromFile("properties.json");
            test_eq(colRef->size(), 2, "");
            test_eq(colRef->findById(".MyProgressBar")->get("size_y"), "25", "");
            colRef->loadJsonFromFile("properties2.json");
            test_eq(colRef->findById(".MyProgressBar")->get("size_y"), "30", "");
        TEST_END
    }

    void run(){
        testModel<ofxCMS::Model>();
        testModel<ofxCMS::ExtendedModel>();
        testExtendedModel<ofxCMS::ExtendedModel>();

        class NothingClass {};
        testObjectCollectionBase<NothingClass>();
        testObjectCollectionBase<ofxCMS::Model>();

        testObjectCollection<NothingClass>();
        testObjectCollection<ofxCMS::Model>();

        testModelCollection<ofxCMS::ModelCollection<ofxCMS::Model>>();
        testModelCollection<ofxCMS::Collection<ofxCMS::Model>>();

        testCollectionAddons();

        TEST_START(collection manager)
            // create singleton collections manager instance
            auto managerRef = ofxCMS::Manager<ofxCMS::Collection<ofxCMS::Model>>::singletonRef();
            // load data from json, which automatically creates the necessary collections
            managerRef->loadJsonFromFile("manager_data.json");
            // verify two collections were loaded from the json file
            test_eq(managerRef->size(), 2, "");
            // get a model from the products collection
            test_eq(managerRef->get("products")->at(0)->get("price"), "4.99", "");
        TEST_END

        TEST_START(CustomModel)
            class CustomModel : public ofxCMS::Model {
                public:
                    string foo(){ return get("bar"); }
            };

            ofxCMS::Collection<CustomModel> col;
            auto modelRef = col.create();
            test_eq(modelRef->foo(), "", "");

            col.modelChangeEvent.addListener([](CustomModel& model){
                model.set("lambda", "called");
            }, this);

            test_eq(modelRef->get("lambda"), "", "");
            modelRef->set("some", "change");
            test_eq(modelRef->get("lambda"), "called", "");
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
