# ESPHome LEDC_sleep custom components

The custome (or excternal) comonents is an extension of the ledc platform and enables PWM support in ESP32 Light_Sleep. to do this the Clock Source needs to be set to a light_Sleep compatible clock and the sleep_mode needs to be set to keep_alive. 

This custom components enables these two parameters.
Other than this, the ledc_sleep platform behaves like the ledc platform and it interfaces to Light as per usual.

once esphome fully supports expressif 5.5.1 t and IF these parameters are exposed by esphome, then this custom component will no longer be required.


