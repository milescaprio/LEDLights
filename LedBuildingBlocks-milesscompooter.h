#pragma once
#include "TickLEDs.h"
#include "ShaderLEDs.h"
class LightPoint : public TickLEDs::LEDEntity { //Abstract class
public:
    //consts
    virtual float velocity() = 0; //LEDs per second, virtual override can access protected members in this class
    virtual float intensity_distance(float dist) = 0; //Yes, signed
    virtual bool kill_condition() = 0;
    virtual Colors::RGBu8 color() = 0;
    virtual Colors::RGBu8 intensity(Colors::RGBu8, float multiplier) = 0;

protected: //Could construct these down but better for extension classes to handle it
    int left_max_radius; //These keep it optimized for smaller points to not have to evaluate all the leds
    int right_max_radius;
    int led_count;
    int iter_i = 0;
    
    //active
protected:
    float location_raw; //Not looped
    float location; //Need to stay in protected scope because these values dont have other ways of being acessed: tick_function override is now consumed. 
    float elapsed;

    //definitions
    virtual TickLEDs::LEDColor get_next() final override;
    virtual void reset_iter() final override;
    virtual void tick_function(float dt) override;
protected:
    LightPoint() = default;
public:
    LightPoint(const LightPoint& other) = default;
    LightPoint(LightPoint&& other) = default;
};

class MovingSource : public LightPoint { 
private: 
    float const_velocity;
    float ISQLaw_coeff;
    float location_start;
    float location_end;
    float grace_distance;
    Colors::RGBu8 const_color;
public:
    virtual float velocity() override; //LEDs per second
    virtual float intensity_distance(float dist) override; //Yes, signed
    virtual bool kill_condition() override;
    virtual Colors::RGBu8 color() override;
    virtual Colors::RGBu8 intensity(Colors::RGBu8, float multiplier) override;
    MovingSource(float velocity, Colors::RGBu8 color, float isqlaw_coeff, float location_start, float location_end, float grace_distance, int left_max, int right_max, int leds);
    MovingSource(const MovingSource& other) = default;
    MovingSource(MovingSource&& other) = default;
};

class MissleMovingSource : public MovingSource {
    float redshift_ratio = 1.0; //represents how much stronger ISQ law is on direction that the missle is moving; imprecise terminology
public:
    virtual float intensity_distance(float dist) override;
    explicit MissleMovingSource(float velocity, Colors::RGBu8 color, float isqlaw_coeff, int redshift_ratio, float grace_distance, int left_max, int right_max, int leds) : MovingSource(velocity, color, isqlaw_coeff, 0, leds, grace_distance, left_max, right_max, leds) {
        this->redshift_ratio = redshift_ratio;
    }
};

class ConstantLight : public ShaderLEDs {
private:
    Colors::RGBu8 color;
public:
    explicit ConstantLight(Colors::RGBu8 color, u8* stream, int led_count) : ShaderLEDs(stream, led_count) {
        this->color = color;
    }
    ConstantLight() = default;
    Colors::RGBu8 poll(float time_ms, int i) override;
};

class MissleStarter : public TickLEDs {
public:
    /*
    Probability models can be derived from a poisson distribution.However,
    unlike a poisson distribution, the missle locations are rounded up to the locations at which the ticks
    were called. This also means that on outlier long ticks, only one missle can be fired, despite the 
    distribution calling for more. It also means that if there is a probability too high for the ticks,
    the granular approximation of the poisson distribution may visually cluster with the ticks, making
    a repeating pattern.
    */
    std::function<float(float dt, float elapsed)> missleProbability; 
    std::function<Colors::RGBu8(float elapsed)> missleColor;
    std::function<float(float elapsed)> missleISQCoeff;
    std::function<float(float elapsed)> missleRedshiftRatio;
    std::function<float(float elapsed)> missleVelocity;
    void entity_handler(float dt) override;
    explicit MissleStarter(u8* stream, int led_count, std::function<float(float dt, float elapsed)> missleProbability, std::function<Colors::RGBu8(float elapsed)> missleColor, std::function<float(float elapsed)> missleISQCoeff, std::function<float(float elapsed)> missleRedshiftRatio, std::function<float(float elapsed)> missleVelocity) : TickLEDs(stream, led_count, TickLEDs::asymptotic_add_blending) {
        this->missleProbability = missleProbability;
        this->missleISQCoeff = missleISQCoeff;
        this->missleColor = missleColor;
        this->missleRedshiftRatio = missleRedshiftRatio;
        this->missleVelocity = missleVelocity;
    }
};