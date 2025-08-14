#ifndef ISPIRE_RUNTIME_H
#define ISPIRE_RUNTIME_H

#include <string>
#include <vector>
#include <map>

struct Bounds {
    float x; 
    float y;
    float width;
    float height;
}; 

class ISpineRuntime {
public:
    virtual void init(const std::string& atlas_path, const std::string& skeleton_path) = 0;
    virtual std::vector<std::string> getAllSkins() = 0; 
    virtual std::map<std::string, float> getAllAnimations() = 0;
    virtual Bounds getBounds() = 0; 
    virtual void setDefaultMix(float mix) = 0;
    virtual void setPosition(float x, float y) = 0;
    virtual void setScale(float scale) = 0;
    virtual void setAnimation(const std::string& animation_name) = 0;
    virtual void setSkin(const std::string& skin_name) = 0;
    virtual void createRenderer() = 0; 
    virtual void setViewportSize(int width, int height) = 0;
    virtual void update(float delta_time) = 0;
    virtual void draw(bool pma) = 0;
    virtual void dispose() = 0;
    virtual ~ISpineRuntime() = default;
}; 

#endif // ISPIRE_RUNTIME_H