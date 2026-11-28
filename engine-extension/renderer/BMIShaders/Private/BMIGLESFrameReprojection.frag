#version 300 es
precision highp float;

uniform sampler2D u_CurrentFrame;
uniform sampler2D u_PreviousFrame;
uniform sampler2D u_MotionVectors;

// Motion vectors are UV-space displacements from the current pixel to its
// corresponding location in the previous frame (previousUV - currentUV).
// phase is the forward prediction amount: 0.0 = current frame,
// 0.5 = halfway to the next frame, 1.0 = one full source-frame interval.
uniform float u_Phase;
uniform float u_MotionScale;
uniform float u_ValidationThreshold;

in vec2 v_UV;
layout(location = 0) out vec4 o_ExtrapolatedFrame;

bool InBounds(vec2 uv)
{
    return all(greaterThanEqual(uv, vec2(0.0))) &&
           all(lessThanEqual(uv, vec2(1.0)));
}

void main()
{
    vec2 motionToPrevious = texture(u_MotionVectors, v_UV).rg * u_MotionScale;

    // Forward prediction: move the current image opposite the
    // current-to-previous motion vector.
    vec2 predictedSourceUV = v_UV + motionToPrevious * u_Phase;
    vec4 currentAtPixel = texture(u_CurrentFrame, v_UV);

    if (!InBounds(predictedSourceUV))
    {
        o_ExtrapolatedFrame = currentAtPixel;
        return;
    }

    vec4 predicted = texture(u_CurrentFrame, predictedSourceUV);

    // Validate the motion field against the previous frame. This is a
    // lightweight confidence heuristic, not a full disocclusion solution.
    vec2 previousUV = v_UV + motionToPrevious;
    float confidence = 0.0;
    if (InBounds(previousUV))
    {
        vec3 previous = texture(u_PreviousFrame, previousUV).rgb;
        float errorValue = length(previous - currentAtPixel.rgb);
        confidence = 1.0 - smoothstep(
            u_ValidationThreshold,
            u_ValidationThreshold * 2.0,
            errorValue
        );
    }

    // When temporal validation is poor, fall back toward the current frame
    // rather than amplifying a bad motion vector.
    float weight = clamp(confidence, 0.0, 1.0);
    o_ExtrapolatedFrame = mix(currentAtPixel, predicted, weight);
}
