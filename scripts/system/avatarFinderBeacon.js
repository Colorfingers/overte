//  avatarFinderBeacon.js
//
//  Created by Thijs Wenker on December 7th, 2016
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2023 Overte e.V.
//
//  Shows 2km long red beams for each avatar outside of the 20 meter radius of your avatar, tries to ignore AC Agents.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var MIN_DISPLAY_DISTANCE = 20.0; // meters
var BEAM_COLOR = {"red": 255, "green": 0, "blue": 0};
var SHOW_THROUGH_WALLS = false;
var BEACON_LENGTH = 2000.0; // meters
var TRY_TO_IGNORE_AC_AGENTS = true;

var HALF_BEACON_LENGTH = BEACON_LENGTH / 2.0;

var beacons = {};

// List of .fst files used by AC scripts, that should be ignored in the script in case TRY_TO_IGNORE_AC_AGENTS is enabled
var POSSIBLE_AC_AVATARS = [
    Script.getExternalPath(Script.ExternalPaths.HF_Content, "/ozan/dev/avatars/invisible_avatar/invisible_avatar.fst"),
    Script.getExternalPath(Script.ExternalPaths.HF_Content, "/ozan/dev/avatars/camera_man/pod/_latest/camera_man_pod.fst")
];

AvatarFinderBeacon = function(avatar) {
    var visible = false;
    var avatarSessionUUID = avatar.sessionUUID;
    var renderLayer = "world";
    if (SHOW_THROUGH_WALLS) { 
        renderLayer = "front"; 
    }
    this.overlay = Entities.addEntity({
        "type": "PolyLine",
        "color": BEAM_COLOR,
        "linePoints": [
            Vec3.sum(avatar.position, {"x": 0, "y": -HALF_BEACON_LENGTH, "z": 0}),
            Vec3.sum(avatar.position, {"x": 0, "y": HALF_BEACON_LENGTH, "z": 0})
        ],
        "strokeWidths": [
            0.02,
            0.02
        ],
        "rotation": {"x": 0, "y": 0, "z": 0, "w": 1},
        "visible": visible,
        "renderLayer": renderLayer,
        "ignorePickIntersection": true,
        "parentID": avatarSessionUUID,
        "parentJointIndex": -2
    }, "local");
    this.cleanup = function() {
        Entities.deleteEntity(this.overlay);
    };
    this.shouldShow = function() {
        return Vec3.distance(MyAvatar.position, avatar.position) >= MIN_DISPLAY_DISTANCE;
    };
    this.update = function() {
        avatar = AvatarList.getAvatar(avatarSessionUUID);
        Entities.editEntity(this.overlay, {
            "visible": this.shouldShow()
        });
    };
};

function updateBeacon(avatarSessionUUID) {
    if (!(avatarSessionUUID in beacons)) {
        var avatar = AvatarList.getAvatar(avatarSessionUUID);
        if (TRY_TO_IGNORE_AC_AGENTS
            && (POSSIBLE_AC_AVATARS.indexOf(avatar.skeletonModelURL) !== -1 || Vec3.length(avatar.position) === 0.0)) {
            return;
        }
        beacons[avatarSessionUUID] = new AvatarFinderBeacon(avatar);
        return;
    }
    beacons[avatarSessionUUID].update();
}

Window.domainChanged.connect(function () {
    beacons = {};
});

Script.update.connect(function() {
    AvatarList.getAvatarIdentifiers().forEach(function(avatarSessionUUID) {
        updateBeacon(avatarSessionUUID);
    });
});

AvatarList.avatarRemovedEvent.connect(function(avatarSessionUUID) {
    if (avatarSessionUUID in beacons) {
        beacons[avatarSessionUUID].cleanup();
        delete beacons[avatarSessionUUID];
    }
});

Script.scriptEnding.connect(function() {
    for (var sessionUUID in beacons) {
        if (!beacons.hasOwnProperty(sessionUUID)) {
            return;
        }
        beacons[sessionUUID].cleanup();
    }
});
