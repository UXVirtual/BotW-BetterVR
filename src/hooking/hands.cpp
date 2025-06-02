#include "../instance.h"
#include "cemu_hooks.h"
#include "hands.h"

void modifyWeaponMtxToVRPose(OpenXR::EyeSide side, BEMatrix34& toBeAdjustedMtx, glm::fquat cameraRotation, glm::fvec3 cameraPosition) {
    // convert VR controller info to glm
    OpenXR::InputState inputs = VRManager::instance().XR->m_input.load();

    glm::fvec3 controllerPos = glm::fvec3(0.0f);
    glm::fquat controllerQuat = glm::identity<glm::fquat>();

    if (inputs.inGame.in_game && inputs.inGame.pose[side].isActive) {
        auto& handPose = side == OpenXR::EyeSide::LEFT ? inputs.inGame.poseLocation[side] : inputs.inGame.poseLocation[side];

        if (handPose.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
            controllerPos = glm::fvec3(
                handPose.pose.position.x,
                handPose.pose.position.y,
                handPose.pose.position.z
            );
        }
        if (handPose.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
            controllerQuat = glm::fquat(
                handPose.pose.orientation.w,
                handPose.pose.orientation.x,
                handPose.pose.orientation.y,
                handPose.pose.orientation.z
            );
        }
    }

    // handPose.orientation.w = rotateHorizontalCounter.w;
    // handPose.orientation.x = rotateHorizontalCounter.x;
    // handPose.orientation.y = rotateHorizontalCounter.y;
    // handPose.orientation.z = rotateHorizontalCounter.z;
    // rotateHorizontalCounter = glm::rotate(rotateHorizontalCounter, glm::radians(360.0f/30.0f/1.0f), glm::fvec3(1.0f, 0.0f, 0.0f));

    // Next, calculate the rotation
    glm::fquat rotatedControllerQuat = glm::normalize(cameraRotation * controllerQuat);
    rotatedControllerQuat = glm::rotate(rotatedControllerQuat, glm::radians(180.0f), glm::fvec3(1.0f, 0.0f, 0.0f));
    rotatedControllerQuat = glm::rotate(rotatedControllerQuat, glm::radians(180.0f), glm::fvec3(0.0f, 0.0f, 1.0f));
    glm::fmat3 finalMtx = glm::mat3_cast(glm::inverse(rotatedControllerQuat));

    toBeAdjustedMtx.x_x = finalMtx[0][0];
    toBeAdjustedMtx.y_x = finalMtx[0][1];
    toBeAdjustedMtx.z_x = finalMtx[0][2];

    toBeAdjustedMtx.x_y = finalMtx[1][0];
    toBeAdjustedMtx.y_y = finalMtx[1][1];
    toBeAdjustedMtx.z_y = finalMtx[1][2];

    toBeAdjustedMtx.x_z = finalMtx[2][0];
    toBeAdjustedMtx.y_z = finalMtx[2][1];
    toBeAdjustedMtx.z_z = finalMtx[2][2];

    // First, calculate the position
    // Use player position as the origin since we want to overwrite the weapon position with the VR controller position
    glm::fvec3 rotatedControllerPos = cameraRotation * controllerPos;
    glm::fvec3 finalPos = cameraPosition + rotatedControllerPos;

    toBeAdjustedMtx.pos_x = finalPos.x;
    toBeAdjustedMtx.pos_y = finalPos.y;
    toBeAdjustedMtx.pos_z = finalPos.z;
}

void CemuHooks::hook_ChangeWeaponMtx(PPCInterpreter_t* hCPU) {
    hCPU->instructionPointer = hCPU->sprNew.LR;

    // r3 holds the source actor pointer
    // r4 holds the bone name
    // r5 holds the matrix that is to be set
    // r6 holds the extra matrix that is to be set
    // r7 holds the ModelBindInfo->mtx
    // r8 holds the target actor pointer
    // r9 returns 1 if the weapon is modified
    // r10 holds the camera pointer

    hCPU->gpr[9] = 0;

    uint32_t actorPtr = hCPU->gpr[3];
    uint32_t boneNamePtr = hCPU->gpr[4];
    uint32_t weaponMtxPtr = hCPU->gpr[5];
    uint32_t playerMtxPtr = hCPU->gpr[6];
    uint32_t modelBindInfoMtxPtr = hCPU->gpr[7];
    uint32_t targetActorPtr = hCPU->gpr[8];
    uint32_t cameraPtr = hCPU->gpr[10];

    uint32_t actorLinkPtr = actorPtr + offsetof(ActorWiiU, baseProcPtr);
    uint32_t actorNamePtr = 0;
    readMemoryBE(actorLinkPtr, &actorNamePtr);
    if (actorNamePtr == 0)
        return;
    char* actorName = (char*)s_memoryBaseAddress + actorNamePtr;

    BESeadLookAtCamera camera = {};
    readMemory(cameraPtr, &camera);
    glm::fvec3 cameraPos = camera.pos.getLE();
    glm::fvec3 cameraAt = camera.at.getLE();
    glm::fquat lookAtQuat = glm::quatLookAtRH(glm::normalize(cameraAt - cameraPos), { 0.0, 1.0, 0.0 });
    glm::fvec3 lookAtPos = cameraPos;
    lookAtPos.y += CemuHooks::GetSettings().playerHeightSetting.getLE();

    // read bone name
    if (boneNamePtr == 0)
        return;
    char* boneName = (char*)s_memoryBaseAddress + boneNamePtr;

    // real logic
    bool isHeldByPlayer = strcmp(actorName, "GameROMPlayer") == 0;
    bool isLeftHandWeapon = strcmp(boneName, "Weapon_L") == 0;
    bool isRightHandWeapon = strcmp(boneName, "Weapon_R") == 0;
    if (actorName[0] != '\0' && boneName[0] != '\0' && isHeldByPlayer && (isLeftHandWeapon || isRightHandWeapon)) {
        BEMatrix34 weaponMtx = {};
        readMemory(weaponMtxPtr, &weaponMtx);

        BEMatrix34 modelBindInfoMtx = {};
        readMemory(modelBindInfoMtxPtr, &modelBindInfoMtx);

        modifyWeaponMtxToVRPose(isLeftHandWeapon ? OpenXR::EyeSide::LEFT : OpenXR::EyeSide::RIGHT, weaponMtx, lookAtQuat, lookAtPos);

        // prevent weapon transparency
        BEType<float> modelOpacity = 1.0f;
        writeMemory(targetActorPtr + offsetof(ActorWiiU, modelOpacity), &modelOpacity);
        uint8_t opacityOrSomethingEnabled = 1;
        writeMemory(targetActorPtr + offsetof(ActorWiiU, opacityOrSomethingEnabled), &opacityOrSomethingEnabled);

        writeMemory(weaponMtxPtr, &weaponMtx);
        writeMemory(modelBindInfoMtxPtr, &modelBindInfoMtx);

        hCPU->gpr[9] = 1;
    }
}

// some ideas for improvements:
// - movement inaccuracy for more difficulty


struct MotionSample {
    glm::fvec3 position;
    glm::fquat rotation;
    glm::fvec3 linearVelocity;
    glm::fvec3 angularVelocity;
};

struct SmallSwordProfile {

};

const int MAX_SAMPLES = 300;

class NewWeaponMotionAnalyser {
public:
    NewWeaponMotionAnalyser() = default;

    void Update(XrSpaceLocation handLocation, XrSpaceVelocity handVelocity) {
        // turn it into a sample
        MotionSample sample = {
            .position = glm::fvec3(handLocation.pose.position.x, handLocation.pose.position.y, handLocation.pose.position.z),
            .rotation = glm::fquat(handLocation.pose.orientation.w, handLocation.pose.orientation.x, handLocation.pose.orientation.y, handLocation.pose.orientation.z),
            .linearVelocity = glm::fvec3(handVelocity.linearVelocity.x, handVelocity.linearVelocity.y, handVelocity.linearVelocity.z),
            .angularVelocity = glm::fvec3(handVelocity.angularVelocity.x, handVelocity.angularVelocity.y, handVelocity.angularVelocity.z)
        };
        m_samples[m_currentSampleIndex] = sample;
        m_currentSampleIndex = (m_currentSampleIndex + 1) % MAX_SAMPLES;
    }

    void Reset(WeaponType weaponType) {
        m_samples.fill({});
    }

    void ResetIfWeaponTypeChanged(WeaponType weaponType) {
        if (m_weaponType != weaponType) {
            m_weaponType = weaponType;
            Reset(weaponType);
        }
    }

    void DrawDebugOverlay() {
        static int viewMode = 0; // 0: Top, 1: Side
        if (ImGui::Button("Top View")) viewMode = 0;
        ImGui::SameLine();
        if (ImGui::Button("Side View")) viewMode = 1;
        ImGui::SameLine();
        ImGui::Text("Weapon Type: %d", (int)m_weaponType);

        // Prepare data arrays
        std::vector<float> xs, ys, zs;
        std::vector<ImU32> linVelColors;
        std::vector<ImU32> angVelColors;
        float maxLinVel = 0.0f, maxAngVel = 0.0f;
        for (const auto& sample : m_samples) {
            float linVelMag = glm::length(sample.linearVelocity);
            float angVelMag = glm::length(sample.angularVelocity);
            if (linVelMag > maxLinVel) maxLinVel = linVelMag;
            if (angVelMag > maxAngVel) maxAngVel = angVelMag;
        }
        for (const auto& sample : m_samples) {
            xs.push_back(sample.position.x);
            ys.push_back(sample.position.y);
            zs.push_back(sample.position.z);
            float linVelMag = glm::length(sample.linearVelocity);
            float angVelMag = glm::length(sample.angularVelocity);
            float tLin = maxLinVel > 0.0f ? linVelMag / maxLinVel : 0.0f;
            float tAng = maxAngVel > 0.0f ? angVelMag / maxAngVel : 0.0f;
            // Linear velocity: blue (low) to red (high)
            ImVec4 linColor = ImVec4(1.0f * tLin, 0.0f, 1.0f - tLin, 1.0f);
            linVelColors.push_back(ImGui::ColorConvertFloat4ToU32(linColor));
            // Angular velocity: green (low) to yellow (high)
            ImVec4 angColor = ImVec4(tAng, tAng, 0.0f, 1.0f); // green to yellow
            angVelColors.push_back(ImGui::ColorConvertFloat4ToU32(angColor));
        }

        ImVec2 plotSize = ImVec2(400, 400);
        if (ImPlot3D::BeginPlot("Weapon Motion", plotSize)) {
            ImPlot3D::SetupAxes("X", "Y", "Z");
            if (viewMode == 0) {
                ImPlot3D::SetupAxisLimits(ImAxis3D_X, -1, 1, ImPlot3DCond_Once);
                ImPlot3D::SetupAxisLimits(ImAxis3D_Y, -1, 1, ImPlot3DCond_Once);
                ImPlot3D::SetupAxisLimits(ImAxis3D_Z, -1, 1, ImPlot3DCond_Once);
            } else {
                ImPlot3D::SetupAxisLimits(ImAxis3D_X, -1, 1, ImPlot3DCond_Once);
                ImPlot3D::SetupAxisLimits(ImAxis3D_Y, -1, 1, ImPlot3DCond_Once);
                ImPlot3D::SetupAxisLimits(ImAxis3D_Z, -1, 1, ImPlot3DCond_Once);
            }
            if (!xs.empty()) {
                // Draw the path as a line colored by linear velocity
                ImDrawList* drawList = ImPlot3D::GetPlotDrawList();
                for (size_t i = 1; i < xs.size(); ++i) {
                    ImVec2 p0 = ImPlot3D::PlotToPixels(xs[i-1], ys[i-1], zs[i-1]);
                    ImVec2 p1 = ImPlot3D::PlotToPixels(xs[i], ys[i], zs[i]);
                    drawList->AddLine(p0, p1, linVelColors[i], 2.0f);
                }
                // Draw scatter points colored by angular velocity
                for (size_t i = 0; i < xs.size(); ++i) {
                    ImVec2 pix = ImPlot3D::PlotToPixels(xs[i], ys[i], zs[i]);
                    drawList->AddCircleFilled(pix, 3.0f, angVelColors[i]);
                }
            }
            ImPlot3D::EndPlot();
        }
    }

private:
    WeaponType m_weaponType = WeaponType::UnknownWeapon;

    std::array<MotionSample, MAX_SAMPLES> m_samples = {};
    int m_currentSampleIndex = 0;
};


// we should have two Motion Analysers here, since we have two hands
std::array<NewWeaponMotionAnalyser, 2> s_motionAnalyzers = {};

void DrawDebugOverlays() {
    if (ImGui::Begin("Weapon Motion Debugger")) {
        for (auto& analyzer : s_motionAnalyzers) {
            ImGui::PushID(&analyzer);
            analyzer.DrawDebugOverlay();
            ImGui::PopID();
        }
        ImGui::End();
    }
}

void CemuHooks::hook_EnableWeaponAttackSensor(PPCInterpreter_t* hCPU) {
    hCPU->instructionPointer = hCPU->sprNew.LR;

    uint32_t weaponPtr = hCPU->gpr[3];
    uint32_t parentActorPtr = hCPU->gpr[4];
    uint32_t heldIndex = hCPU->gpr[5]; // this is either 0 or 1 depending on which hand the weapon is in
    bool isHeldByPlayer = hCPU->gpr[6] == 0;

    Weapon weapon = {};
    readMemory(weaponPtr, &weapon);

    WeaponType weaponType = weapon.type.getLE();
    if (weaponType == WeaponType::Bow || weaponType == WeaponType::Shield) {
        Log::print("! Skipping motion analysis for Bow/Shield (type: {})", (int)weaponType);
        return;
    }

    if (heldIndex >= s_motionAnalyzers.size()) {
        Log::print("! Invalid heldIndex: {}. Skipping motion analysis.", heldIndex);
        return;
    }

    auto state = VRManager::instance().XR->m_input.load();
    s_motionAnalyzers[heldIndex].ResetIfWeaponTypeChanged(weaponType);
    s_motionAnalyzers[heldIndex].Update(state.inGame.poseLocation[heldIndex], state.inGame.poseVelocity[heldIndex]);

    // 0x3A0 is 0 apparently for armor displays, so prevent null pointer dereference
    //ActorWiiU parentActor = {};
    //readMemory(parentActorPtr, &parentActor);
    //bool hasPhysics = parentActor.actorPhysicsPtr.getLE() != 0;
    // Log::print("! parentActorPtr=0x{:X}, hasPhysics={}", parentActorPtr, hasPhysics);

    // Use the analysed motion to determine whether the weapon is swinging or stabbing, and whether the attackSensor should be active this frame
    if (isHeldByPlayer && s_motionAnalyzers[heldIndex].IsAttacking()) {
        Log::print("!! Activate sensor for {}: isHeldByPlayer={}, weaponType={}", heldIndex, isHeldByPlayer, (int)weaponType);
        weapon.setupAttackSensor.resetAttack = 1;
        weapon.setupAttackSensor.mode = 2;
        weapon.setupAttackSensor.isContactLayerInitialized = 0;
        // weapon.setupAttackSensor.overrideImpact = 1;
        // weapon.setupAttackSensor.multiplier = analyzer->GetDamage();
        // weapon.setupAttackSensor.impact = analyzer->GetImpulse();
        writeMemory(weaponPtr, &weapon);
    }
    else {
        // Log::print("! Not activating attack sensor: isHeldByPlayer={}, hasPhysics={}", isHeldByPlayer, hasPhysics);
    }
}

void CemuHooks::hook_EquipWeapon(PPCInterpreter_t* hCPU) {
    hCPU->instructionPointer = hCPU->sprNew.LR;

    auto input = VRManager::instance().XR->m_input.load();
    uint32_t toBeEquipedSlot = hCPU->gpr[25];

    if (toBeEquipedSlot == 0 || toBeEquipedSlot == 1) {
        hCPU->gpr[25] = (input.inGame.lastPickupSide == OpenXR::EyeSide::LEFT) ? 0 : 1;
    }

    //switch (hCPU->gpr[25]) {
    //    case 1: {
    //        hCPU->gpr[25] = 0;
    //        break;
    //    }
    //    case 0: {
    //        hCPU->gpr[25] = 1;
    //    }
    //    default: {
    //        break;
    //    }
    //}
}

void CemuHooks::hook_ModifyHandModelAccessSearch(PPCInterpreter_t* hCPU) {
    hCPU->instructionPointer = hCPU->sprNew.LR;

    if (hCPU->gpr[3] == 0) {
        return;
    }
#ifdef _DEBUG
    // r3 holds the address of the string to search for
    const char* actorName = (const char*)(s_memoryBaseAddress + hCPU->gpr[3]);

    if (actorName != nullptr) {
        // Weapon_R is presumably his right hand bone name
        Log::print("! Searching for model handle using {}", actorName);
    }
#endif
}

void CemuHooks::hook_CreateNewActor(PPCInterpreter_t* hCPU) {
    hCPU->instructionPointer = hCPU->sprNew.LR;

    // if (VRManager::instance().XR->GetRenderer() == nullptr || VRManager::instance().XR->GetRenderer()->m_layer3D.GetStatus() == RND_Renderer::Layer3D::Status3D::UNINITIALIZED) {
    //     hCPU->gpr[3] = 0;
    //     return;
    // }
    hCPU->gpr[3] = 0;

    // OpenXR::InputState inputs = VRManager::instance().XR->m_input.load();
    // if (!inputs.inGame.in_game) {
    //     hCPU->gpr[3] = 0;
    //     return;
    // }
    //
    // // test if controller is connected
    // if (inputs.inGame.grab[OpenXR::EyeSide::LEFT].currentState == XR_TRUE && inputs.inGame.grab[OpenXR::EyeSide::LEFT].changedSinceLastSync == XR_TRUE) {
    //     Log::print("Trying to spawn new thing!");
    //     hCPU->gpr[3] = 1;
    // }
    // else if (inputs.inGame.grab[OpenXR::EyeSide::RIGHT].currentState == XR_TRUE && inputs.inGame.grab[OpenXR::EyeSide::RIGHT].changedSinceLastSync == XR_TRUE) {
    //     Log::print("Trying to spawn new thing!");
    //     hCPU->gpr[3] = 1;
    // }
    // else {
    //     hCPU->gpr[3] = 0;
    // }
}

