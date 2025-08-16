#include "ISpineRuntime.h"
#include "spine-opengl.h"

using namespace spine;

class SpineRuntime : public ISpineRuntime {
public:
    SpineRuntime() {
        // Constructor implementation
    }

    bool init(const std::string& atlas_path, const std::string& skeleton_path) override {
        // Initialize the spine runtime with the provided atlas and skeleton paths
        atlas = new Atlas(atlas_path.c_str(), &textureLoader);
        if (skeleton_path.ends_with(".json")) {
            SkeletonJson json(atlas);
            skeletonData = json.readSkeletonDataFile(skeleton_path.c_str());
        } else if (skeleton_path.ends_with(".skel")) {
            SkeletonBinary binary(atlas);
            skeletonData = binary.readSkeletonDataFile(skeleton_path.c_str());
        }
        if (skeletonData == nullptr) return false;
        skeleton = new Skeleton(skeletonData);
        stateData = new AnimationStateData(skeletonData);
        state = new AnimationState(stateData);
        return true; 
    }

    std::vector<std::string> getAllSkins() override {
        // Return all skin names from the skeleton data
        std::vector<std::string> skinNames;
        auto skins = skeletonData->getSkins();
        auto default_skin = skeletonData->getDefaultSkin(); 
        if (skins.size() <= 1) skinNames.push_back(default_skin->getName().buffer()); 
        else 
            for (size_t i = 0; i < skins.size(); ++i)
                if (skins[i] != default_skin)
                    skinNames.push_back(skins[i]->getName().buffer());
        return skinNames;
    }

    std::map<std::string, float> getAllAnimations() override {
        // Return all animation names from the skeleton data
        std::map<std::string, float> animationNames;
        auto animations = skeletonData->getAnimations(); 
        for (size_t i = 0; i < animations.size(); ++i)
            animationNames[animations[i]->getName().buffer()] = animations[i]->getDuration();
        return animationNames;
    }

    Bounds getBounds() override {
        float l = 0.0, r = 0.0, b = 0.0, t = 0.0; 
        float x, y, w, h; 
        Vector<float> vertices;
        Vector<Animation*> animations = skeletonData->getAnimations();
        for (size_t i = 0; i < animations.size(); i++) {
            setAnimation(animations[i]->getName().buffer());
            setPosition(0, 0);
            setScale(1.0f);
            while (!state->getCurrent(0)->isComplete()) {
                update(0.04f); 
                skeleton->getBounds(x, y, w, h, vertices);
                l = std::min(l, x);
                r = std::max(r, w + x);
                b = std::min(b, y);
                t = std::max(t, h + y);
            }
        }
        return { l, b, r - l, t - b };
    }

    void setDefaultMix(float mix) override {
        // Set the default mix for animations
        stateData->setDefaultMix(mix);
    }

    void setPosition(float x, float y) override {
        // Set the position of the spine object
        skeleton->setPosition(x, y);
    }

    void setScale(float scale) override {
        // Set the scale of the spine object
        skeleton->setScaleX(scale);
        skeleton->setScaleY(scale);
    }

    void setAnimation(const std::string& animation_name) override {
        // Set the specified animation with looping option
        state->setAnimation(0, animation_name.c_str(), true);
    }

    void setSkin(const std::string& skin_name) override {
        // Set the specified skin for the skeleton
        skeleton->setSkin(skin_name.c_str());
        skeleton->setSlotsToSetupPose(); 
    }

    void createRenderer() override {
        // Create the renderer for rendering spine objects
        renderer = renderer_create(); 
    }

    void setViewportSize(int width, int height) override {
        // Set the viewport size for rendering
        renderer_set_viewport_size(renderer, width, height);
    }

    void update(float delta_time) override {
        // Update the spine runtime with the elapsed time
        state->update(delta_time);
        state->apply(*skeleton);
#if defined(SPINE37) || defined(SPINE38) || defined(SPINE40) || defined(SPINE42)
        skeleton->update(delta_time); 
#elif defined(SPINE41)
#endif
#if defined(SPINE37) || defined(SPINE38) || defined(SPINE40) || defined(SPINE41)
        skeleton->updateWorldTransform();
#elif defined(SPINE42)
        skeleton->updateWorldTransform(Physics_Update);
#endif
    }

    void draw(bool pma) override {
        // Draw the spine objects with optional premultiplied alpha
        renderer_draw(renderer, skeleton, pma);
    }

    void dispose() override {
        // Dispose of resources used by the spine runtime
        if (renderer) renderer_dispose(renderer);
        if (state) delete state;
        if (stateData) delete stateData;
        if (skeleton) delete skeleton;
        if (skeletonData) delete skeletonData;
        if (atlas) delete atlas;
        renderer = nullptr;
        state = nullptr;
        stateData = nullptr;
        skeleton = nullptr;
        skeletonData = nullptr;
        atlas = nullptr;
    }

    ~SpineRuntime() override {
        // Destructor implementation
        dispose();
    }

private:
    GlTextureLoader textureLoader;
    Atlas* atlas = nullptr;
    SkeletonData* skeletonData = nullptr;
    Skeleton* skeleton = nullptr;
    AnimationStateData* stateData = nullptr;
    AnimationState* state = nullptr;
    renderer_t* renderer = nullptr;
};

#if defined(SPINE37)
extern "C" __declspec(dllexport) ISpineRuntime* createSpineRuntime37() {
    return new SpineRuntime();
}
#elif defined(SPINE38)
extern "C" __declspec(dllexport) ISpineRuntime* createSpineRuntime38() {
    return new SpineRuntime();
}
#elif defined(SPINE40)
extern "C" __declspec(dllexport) ISpineRuntime* createSpineRuntime40() {
    return new SpineRuntime();
}
#elif defined(SPINE41)
extern "C" __declspec(dllexport) ISpineRuntime* createSpineRuntime41() {
    return new SpineRuntime();
}
#elif defined(SPINE42)
extern "C" __declspec(dllexport) ISpineRuntime* createSpineRuntime42() {
    return new SpineRuntime();
}
#endif
