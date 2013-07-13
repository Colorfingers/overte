//
//  ParticleSystem.h
//  hifi
//
//  Created by Jeffrey on July 10, 2013
//
//

#ifndef hifi_ParticleSystem_h
#define hifi_ParticleSystem_h

#include <glm/gtc/quaternion.hpp>

const int MAX_PARTICLES = 10000;
const int MAX_EMITTERS  = 10;

class ParticleSystem {
public:
    ParticleSystem();
    
    int  addEmitter(); // add (create) an emitter and get its unique id
    void emitParticlesNow(int e, int numParticles, float radius, glm::vec4 color, glm::vec3 velocity, float lifespan);
    void simulate(float deltaTime);
    void render();
    void runSpecialEffectsTest(float deltaTime); // for debugging and artistic exploration
     
    void setOrangeBlueColorPalette(); // apply a nice preset color palette to the particles
    void setCollisionSphere(glm::vec3 position, float radius);  // specify a sphere for the particles to collide with
    void setEmitterPosition(int e, glm::vec3 position) { _emitter[e].position = position; } // set the position of this emitter
    void setEmitterRotation(int e, glm::quat rotation) { _emitter[e].rotation = rotation; } // set the rotation of this emitter
    void setUpDirection(glm::vec3 upDirection) {_upDirection = upDirection;} // tell particle system which direction is up
    
private:

    struct Emitter {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 right;
        glm::vec3 up;
        glm::vec3 front;
    };  

    struct Particle {
        bool        alive;        // is the particle active?
        glm::vec3   position;     // position
        glm::vec3   velocity;     // velocity
        glm::vec4   color;        // color (rgba)
        float       age;          // age in seconds
        float       radius;       // radius
        float       lifespan;     // how long this particle stays alive (in seconds)
        int         emitterIndex; // which emitter created this particle?
    };  
        
    glm::vec3  _upDirection;
    float      _bounce;
    float      _gravity;
    float      _timer;
    Emitter    _emitter[MAX_EMITTERS];
    Particle   _particle[MAX_PARTICLES];
    int        _numParticles;
    int        _numEmitters;
    float      _airFriction;
    float      _jitter;
    float      _emitterAttraction;
    float      _tornadoForce;
    float      _neighborAttraction;
    float      _neighborRepulsion;
    bool       _usingCollisionSphere;
    glm::vec3  _collisionSpherePosition;
    float      _collisionSphereRadius;
    
    // private methods
    void updateEmitter(int e, float deltaTime);
    void updateParticle(int index, float deltaTime);
    void createParticle(glm::vec3 position, glm::vec3 velocity, float radius, glm::vec4 color, float lifespan);
    void killParticle(int p);
    void renderEmitter(int emitterIndex, float size);
    void renderParticle(int p);
};

#endif
