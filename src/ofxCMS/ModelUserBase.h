namespace ofxCMS {
    template<class ModelClass>
    class ModelUserBase {
    public:
        ModelUserBase() : modelRef(nullptr){}
        void setup(shared_ptr<ModelClass> modelRef){ this->modelRef = modelRef; }

    private:
        shared_ptr<ModelClass> modelRef;
    };
}
