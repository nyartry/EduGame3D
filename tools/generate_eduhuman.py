"""Create EduHuman from mathematical primitives and original keyframes.

Run with Blender 5.1, for example:
  blender --background --python tools/generate_eduhuman.py

No downloaded character, image, rig, or motion is used as input. This source
was prepared with an AI coding assistant for this educational project; it is
not a claim that a human modelled or animated every part manually.
The editable .blend contains three actions and named, rigidly weighted parts.
Blender coordinates: metres, Z up, -Y forward. FBX: Y up, -Z forward.
"""

import argparse
import math
from pathlib import Path
import sys

import bpy
from mathutils import Euler, Matrix, Quaternion, Vector


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Content" / "Models" / "EduHuman"
FPS = 30
BONE_SPECS = []
PARTS = []
MATERIALS = {}


def bone(name, head, tail, parent=None):
    BONE_SPECS.append((name, Vector(head), Vector(tail), parent))


def material(name, rgb, metallic=0.0, roughness=0.48):
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*rgb, 1.0)
    result.use_nodes = True
    shader = result.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*rgb, 1.0)
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    MATERIALS[name] = result
    return result


def finish_part(obj, name, bone_name, mat):
    obj.name = name
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.data.materials.append(MATERIALS[mat])
    group = obj.vertex_groups.new(name=bone_name)
    group.add(list(range(len(obj.data.vertices))), 1.0, "REPLACE")
    obj["joint"] = bone_name
    obj["construction"] = "Original procedural geometric part; rigid skin weight 1.0"
    PARTS.append(obj)
    return obj


def box(name, center, size, joint, mat, bevel=0.015, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.dimensions = size
    if rotation:
        obj.rotation_euler = rotation
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel:
        modifier = obj.modifiers.new("Small rounded edge", "BEVEL")
        modifier.width = bevel
        modifier.segments = 1
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    return finish_part(obj, name, joint, mat)


def joint_ball(name, position, radius, joint, mat="Joint_Navy"):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=radius, location=position)
    return finish_part(bpy.context.object, name, joint, mat)


def segment(name, start, end, widths, depths, joint, mat):
    """An eight-sided, tapered limb with narrow end rims and flat caps."""
    start, end = Vector(start), Vector(end)
    axis = end - start
    orientation = axis.to_track_quat("Z", "Y").to_matrix()
    # Chamfered rectangular cross-section in local XY.
    section = [(-.68, -1), (.68, -1), (1, -.68), (1, .68),
               (.68, 1), (-.68, 1), (-1, .68), (-1, -.68)]
    rings = [(0, .78), (.10, 1), (.86, 1), (1, .78)]
    vertices = []
    for t, rim in rings:
        w = ((1 - t) * widths[0] + t * widths[1]) * .5 * rim
        d = ((1 - t) * depths[0] + t * depths[1]) * .5 * rim
        for x, y in section:
            vertices.append(start + axis * t + orientation @ Vector((x * w, y * d, 0)))
    faces = [tuple(range(7, -1, -1))]
    for r in range(3):
        for i in range(8):
            j = (i + 1) % 8
            faces.append((r * 8 + i, r * 8 + j, (r + 1) * 8 + j, (r + 1) * 8 + i))
    faces.append(tuple(range(24, 32)))
    mesh = bpy.data.meshes.new(name + "Geometry")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    return finish_part(obj, name, joint, mat)


def create_rig():
    bone("Root", (0, 0, 0), (0, 0, .16))
    bone("Hips", (0, 0, .87), (0, 0, 1), "Root")
    bone("Spine", (0, 0, 1), (0, 0, 1.14), "Hips")
    bone("Chest", (0, 0, 1.14), (0, 0, 1.35), "Spine")
    bone("Neck", (0, 0, 1.35), (0, 0, 1.47), "Chest")
    bone("Head", (0, 0, 1.47), (0, 0, 1.70), "Neck")
    for side, sign in [("L", 1), ("R", -1)]:
        bone("Shoulder." + side, (sign * .03, 0, 1.33), (sign * .245, 0, 1.33), "Chest")
        bone("UpperArm." + side, (sign * .245, 0, 1.33), (sign * .295, 0, 1.08), "Shoulder." + side)
        bone("Forearm." + side, (sign * .295, 0, 1.08), (sign * .33, -.015, .83), "UpperArm." + side)
        bone("Hand." + side, (sign * .33, -.015, .83), (sign * .34, -.015, .74), "Forearm." + side)
        bone("Thigh." + side, (sign * .11, 0, .90), (sign * .11, 0, .51), "Hips")
        bone("Shin." + side, (sign * .11, 0, .51), (sign * .11, 0, .14), "Thigh." + side)
        bone("Foot." + side, (sign * .11, 0, .14), (sign * .11, -.17, .065), "Shin." + side)
        bone("Toe." + side, (sign * .11, -.17, .065), (sign * .11, -.25, .035), "Foot." + side)
    armature = bpy.data.armatures.new("EduHumanSkeleton")
    rig = bpy.data.objects.new("EduHuman", armature)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    for name, head, tail, parent in BONE_SPECS:
        edit = armature.edit_bones.new(name)
        edit.head, edit.tail = head, tail
        if parent:
            edit.parent = armature.edit_bones[parent]
        edit.use_connect = False
        edit.use_deform = True
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.show_in_front = True
    armature.display_type = "OCTAHEDRAL"
    rig["asset_title"] = "EduHuman - original educational humanoid"
    rig["creation_process"] = "Procedurally authored with AI coding assistance; no external model, texture, rig or motion inputs."
    rig["editing_guide"] = "22 named bones. Body parts use explicit rigid weight 1. Edit this script, or pose bones and edit the three actions."
    rig["axes"] = "Blender Z up / -Y forward, metres. FBX Y up / -Z forward."
    rig["animation_guide"] = "Idle 2.4s, Jog 0.8s, Kick 1.6s. All are in place; Root never translates. Kick uses the right leg."
    rig["source_script"] = "tools/generate_eduhuman.py"
    for pose in rig.pose.bones:
        pose.rotation_mode = "QUATERNION"
    return rig


def create_character(rig):
    material("Armor_Teal", (.025, .50, .48), .14)
    material("Joint_Navy", (.023, .046, .080), .10)
    material("Panel_Ivory", (.88, .91, .85), .05)
    material("Accent_Coral", (1.0, .24, .105), .10)
    material("Visor_Dark", (.013, .075, .105), .30, .25)
    material("Light_Mint", (.42, 1.0, .78), .05, .25)
    # Trunk: distinct pelvis, flexible waist, chest shell and neck.
    box("Pelvis shell", (0, 0, .945), (.31, .215, .16), "Hips", "Armor_Teal", .035)
    box("Waist flexible core", (0, 0, 1.06), (.235, .16, .15), "Spine", "Joint_Navy", .025)
    box("Waist buckle", (0, -.116, .969), (.07, .025, .046), "Hips", "Accent_Coral", .009)
    segment("Chest tapered shell", (0, 0, 1.115), (0, 0, 1.36), (.29, .39), (.185, .235), "Chest", "Armor_Teal")
    box("Chest front panel", (0, -.127, 1.268), (.20, .026, .112), "Chest", "Panel_Ivory", .016)
    box("Chest centre badge", (0, -.145, 1.275), (.041, .015, .070), "Chest", "Accent_Coral", .006)
    box("Left chest mark", (.059, -.145, 1.272), (.038, .013, .012), "Chest", "Joint_Navy", .003)
    box("Right chest mark", (-.059, -.145, 1.272), (.038, .013, .012), "Chest", "Joint_Navy", .003)
    box("Upper back panel", (0, .122, 1.265), (.22, .05, .15), "Chest", "Joint_Navy", .020)
    segment("Neck stem", (0, 0, 1.365), (0, 0, 1.478), (.085, .075), (.085, .075), "Neck", "Joint_Navy")
    # A readable, geometric helmet with an original face/visor design.
    box("Head shell", (0, 0, 1.623), (.282, .254, .306), "Head", "Panel_Ivory", .042)
    box("Head crown insert", (0, .007, 1.778), (.119, .175, .029), "Head", "Armor_Teal", .013)
    box("Visor", (0, -.134, 1.641), (.240, .030, .099), "Head", "Visor_Dark", .018)
    for sign, side in [(1, "L"), (-1, "R")]:
        box("Eye lamp " + side, (sign * .058, -.152, 1.645), (.046, .012, .018), "Head", "Light_Mint", .004)
        box("Ear cap " + side, (sign * .149, .004, 1.616), (.040, .102, .099), "Head", "Armor_Teal", .012)
    box("Chin inset", (0, -.128, 1.535), (.091, .018, .032), "Head", "Joint_Navy", .007)
    specs = {n: (h, t) for n, h, t, _ in BONE_SPECS}
    for side, sign in [("L", 1), ("R", -1)]:
        sh, elbow = specs["UpperArm." + side]
        _, wrist = specs["Forearm." + side]
        joint_ball("Shoulder joint " + side, sh, .065, "Shoulder." + side)
        segment("Upper arm armor " + side, sh.lerp(elbow, .16), sh.lerp(elbow, .83), (.122, .103), (.123, .100), "UpperArm." + side, "Armor_Teal")
        joint_ball("Elbow hinge " + side, elbow, .051, "UpperArm." + side)
        segment("Forearm armor " + side, elbow.lerp(wrist, .15), elbow.lerp(wrist, .84), (.113, .084), (.112, .081), "Forearm." + side, "Panel_Ivory")
        segment("Wrist cuff " + side, elbow.lerp(wrist, .84), wrist, (.094, .090), (.090, .089), "Forearm." + side, "Accent_Coral")
        hand_h, hand_t = specs["Hand." + side]
        segment("Glove " + side, hand_h, hand_t + Vector((0, 0, -.018)), (.078, .071), (.092, .081), "Hand." + side, "Joint_Navy")
        box("Glove thumb " + side, hand_h.lerp(hand_t, .52) + Vector((-sign * .046, -.012, .012)), (.037, .057, .056), "Hand." + side, "Armor_Teal", .010)
        hip, knee = specs["Thigh." + side]
        _, ankle = specs["Shin." + side]
        joint_ball("Hip joint " + side, hip, .066, "Hips")
        segment("Thigh armor " + side, hip.lerp(knee, .13), hip.lerp(knee, .85), (.139, .114), (.142, .116), "Thigh." + side, "Armor_Teal")
        joint_ball("Knee hinge " + side, knee, .058, "Thigh." + side)
        box("Knee front guard " + side, knee + Vector((0, -.054, 0)), (.11, .04, .088), "Shin." + side, "Accent_Coral", .018)
        segment("Shin armor " + side, knee.lerp(ankle, .16), knee.lerp(ankle, .83), (.109, .086), (.107, .089), "Shin." + side, "Panel_Ivory")
        segment("Shin teal stripe " + side, knee.lerp(ankle, .26) + Vector((0, -.054, 0)), knee.lerp(ankle, .70) + Vector((0, -.046, 0)), (.025, .023), (.012, .012), "Shin." + side, "Armor_Teal")
        joint_ball("Ankle joint " + side, ankle, .045, "Shin." + side)
        box("Boot " + side, (sign * .11, -.055, .071), (.145, .230, .140), "Foot." + side, "Joint_Navy", .022)
        box("Boot top " + side, (sign * .11, -.072, .142), (.123, .156, .035), "Foot." + side, "Armor_Teal", .012)
        box("Toe cap " + side, (sign * .11, -.204, .045), (.145, .092, .088), "Toe." + side, "Armor_Teal", .016)
        box("Heel safety mark " + side, (sign * .11, .064, .073), (.094, .014, .028), "Foot." + side, "Accent_Coral", .005)
    # Keep original part names in mesh vertex group names and a guide text;
    # a single joined mesh produces the same bind geometry in all exports.
    bpy.ops.object.select_all(action="DESELECT")
    for part in PARTS:
        part.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]
    bpy.ops.object.join()
    mesh = bpy.context.object
    mesh.name = "EduHumanBody"
    # Set mesh origin to the armature origin; transforms remain unit scale.
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    modifier = mesh.modifiers.new("Humanoid skin weights", "ARMATURE")
    modifier.object = rig
    modifier.use_vertex_groups = True
    mesh.parent = rig
    mesh["weighting"] = "Every vertex has one named humanoid bone influence with weight 1.0. Select by vertex group to edit a limb."
    mesh["surface_design"] = "Six original plain-color materials; no texture files or image nodes."
    return mesh


def ease(a, b, t):
    t = max(0.0, min(1.0, t))
    return a + (b - a) * (t * t * (3 - 2 * t))


def curve(t, keys):
    for (ta, a), (tb, b) in zip(keys, keys[1:]):
        if t <= tb:
            return ease(a, b, (t - ta) / (tb - ta))
    return keys[-1][1]


def set_world_bone(rig, name, head, tail, twist=0):
    head, tail = Vector(head), Vector(tail)
    rest = rig.data.bones[name]
    direction = (tail - head).normalized()
    rotation = (rest.tail_local - rest.head_local).normalized().rotation_difference(direction)
    if twist:
        rotation = Quaternion(direction, twist) @ rotation
    matrix = rotation.to_matrix().to_4x4() @ rest.matrix_local
    matrix.translation = head
    pose = rig.pose.bones[name]
    pose.matrix = matrix
    pose.scale = (1, 1, 1)
    bpy.context.view_layer.update()


def two_bone_knee(hip, ankle):
    """Original analytic leg IK, baked to ordinary FK bone keyframes."""
    hip, ankle = Vector(hip), Vector(ankle)
    delta = ankle - hip
    distance = min(.39 + .37 - .0001, max(.03, delta.length))
    direction = delta.normalized()
    # Knees bend towards the front while standing, upwards when kicking.
    preferred = Vector((0, -1, .8))
    perpendicular = (preferred - direction * preferred.dot(direction)).normalized()
    along = (.39 ** 2 - .37 ** 2 + distance ** 2) / (2 * distance)
    height = math.sqrt(max(0, .39 ** 2 - along ** 2))
    # All designed targets are reachable, so the clamp is only a guard.
    if delta.length > .7601:
        raise ValueError(f"Unreachable leg target: {delta.length}")
    return hip + direction * along + perpendicular * height


def apply_pose(rig, clip, t):
    phase = 2 * math.pi * t / 2.4
    hip_x, bob, lean, sway = 0.0, -.012 - .003 * math.sin(phase), 0.0, 0.0
    stride = 2 * math.pi * t / .8
    guard = 0.0
    if clip == "Jog":
        phase = stride
        bob = -.045 + .012 * math.cos(2 * stride)
        hip_x = .012 * math.sin(stride)
        lean, sway = .10, .025 * math.sin(stride)
    elif clip == "Kick":
        phase = 0.0
        guard = curve(t, [(0, 0), (.23, .15), (.50, 1), (.84, 1), (1.22, 0), (1.6, 0)])
        hip_x = .040 * guard
        bob = -.012 - .038 * guard
        lean = -.075 * guard
    offset = Vector((hip_x, 0, bob))
    set_world_bone(rig, "Root", (0, 0, 0), (0, 0, .16))
    set_world_bone(rig, "Hips", offset + Vector((0, 0, .87)), offset + Vector((0, 0, 1)))
    body_rotation = Euler((lean + .008 * math.sin(phase), 0, sway), "XYZ").to_matrix()
    pivot = Vector((0, 0, 1))

    def body_point(point):
        return offset + pivot + body_rotation @ (Vector(point) - pivot)

    set_world_bone(rig, "Spine", body_point((0, 0, 1)), body_point((0, 0, 1.14)))
    set_world_bone(rig, "Chest", body_point((0, 0, 1.14)), body_point((0, 0, 1.35)), .008 * math.sin(phase))
    set_world_bone(rig, "Neck", body_point((0, 0, 1.35)), body_point((0, 0, 1.47)))
    head_h = body_point((0, 0, 1.47))
    head_dir = body_rotation @ Euler((-.02 * guard, .014 * math.sin(phase), 0), "XYZ").to_matrix() @ Vector((0, 0, .23))
    set_world_bone(rig, "Head", head_h, head_h + head_dir, .018 * math.sin(phase))
    for side, sign in [("L", 1), ("R", -1)]:
        shoulder_h = body_point((sign * .03, 0, 1.33))
        shoulder = body_point((sign * .245, 0, 1.33))
        set_world_bone(rig, "Shoulder." + side, shoulder_h, shoulder)
        arm_swing = -.065 + .025 * math.sin(phase + sign * .4)
        elbow_flex = -.13
        if clip == "Jog":
            arm_swing = sign * .44 * math.sin(stride)
            elbow_flex = -.58 - .10 * math.cos(stride * sign)
        elif clip == "Kick":
            arm_swing -= (.40 if side == "L" else .14) * guard
            elbow_flex -= .55 * guard
        arm_rotation = body_rotation @ Euler((arm_swing, 0, 0), "XYZ").to_matrix()
        fore_rotation = body_rotation @ Euler((arm_swing + elbow_flex, 0, 0), "XYZ").to_matrix()
        elbow = shoulder + arm_rotation @ Vector((sign * .05, 0, -.25))
        wrist = elbow + fore_rotation @ Vector((sign * .035, -.015, -.25))
        hand_tail = wrist + fore_rotation @ Vector((sign * .01, 0, -.09))
        set_world_bone(rig, "UpperArm." + side, shoulder, elbow)
        set_world_bone(rig, "Forearm." + side, elbow, wrist)
        set_world_bone(rig, "Hand." + side, wrist, hand_tail)
        ankle = Vector((sign * .11, 0, .14))
        foot_pitch = 0.0
        if clip == "Jog":
            cycle = (t / .8 + (0 if side == "L" else .5)) % 1.0
            if cycle < .5:
                ankle.y = -.15 + .30 * (cycle / .5)
            else:
                swing = (cycle - .5) / .5
                ankle.y = ease(.15, -.15, swing)
                ankle.z += .12 * math.sin(math.pi * swing) ** 1.5
                foot_pitch = -.20 * math.sin(math.pi * swing)
        if clip == "Kick" and side == "R":
            ankle.y = curve(t, [(0, 0), (.24, 0), (.49, -.13), (.68, -.65), (.80, -.65), (1.0, -.11), (1.22, 0), (1.6, 0)])
            ankle.z = curve(t, [(0, .14), (.24, .14), (.49, .56), (.68, .85), (.80, .85), (1.0, .53), (1.22, .14), (1.6, .14)])
            foot_pitch = curve(t, [(0, 0), (.3, 0), (.52, -.1), (.68, -.87), (.8, -.87), (1.0, -.1), (1.22, 0), (1.6, 0)])
        hip = offset + Vector((sign * .11, 0, .90))
        knee = two_bone_knee(hip, ankle)
        set_world_bone(rig, "Thigh." + side, hip, knee)
        set_world_bone(rig, "Shin." + side, knee, ankle)
        foot_rot = Euler((foot_pitch, 0, 0), "XYZ").to_matrix()
        toe_joint = ankle + foot_rot @ Vector((0, -.17, -.075))
        toe_tip = toe_joint + foot_rot @ Vector((0, -.08, -.030))
        set_world_bone(rig, "Foot." + side, ankle, toe_joint)
        set_world_bone(rig, "Toe." + side, toe_joint, toe_tip)


def create_action(rig, clip, seconds):
    rig.animation_data_create()
    action = bpy.data.actions.new(clip)
    action.use_fake_user = True
    action["authorship"] = "Original procedural keyframes generated with AI coding assistance. No external motion samples."
    action["duration_seconds"] = seconds
    action["export_start"] = 1
    action["export_end"] = round(seconds * FPS) + 1
    rig.animation_data.action = action
    previous = {}
    for frame in range(1, round(seconds * FPS) + 2):
        bpy.context.scene.frame_set(frame)
        apply_pose(rig, clip, (frame - 1) / FPS)
        for pose in rig.pose.bones:
            if pose.name in previous and pose.rotation_quaternion.dot(previous[pose.name]) < 0:
                pose.rotation_quaternion.negate()
            previous[pose.name] = pose.rotation_quaternion.copy()
            pose.keyframe_insert("location", frame=frame, group=pose.name)
            pose.keyframe_insert("rotation_quaternion", frame=frame, group=pose.name)
            pose.keyframe_insert("scale", frame=frame, group=pose.name)
    # FBX stores the current pose as the skeleton's default node transforms.
    # A shared rest frame outside the exported clip gives all three files
    # identical node bind transforms, regardless of each animation's first pose.
    bpy.context.scene.frame_set(0)
    for pose in rig.pose.bones:
        pose.matrix_basis = Matrix.Identity(4)
        pose.keyframe_insert("location", frame=0, group=pose.name)
        pose.keyframe_insert("rotation_quaternion", frame=0, group=pose.name)
        pose.keyframe_insert("scale", frame=0, group=pose.name)
    return action


def validate(rig, body, actions):
    assert len(rig.data.bones) == 22
    assert all(abs(v - 1) < 1e-6 for v in rig.scale)
    assert all(abs(v - 1) < 1e-6 for v in body.scale)
    for vertex in body.data.vertices:
        assert len(vertex.groups) == 1
        assert abs(sum(g.weight for g in vertex.groups) - 1) < 1e-6
        assert all(math.isfinite(v) for v in vertex.co)
    assert not bpy.data.images
    for action in actions.values():
        rig.animation_data.action = action
        for frame in range(int(action.frame_range.x), int(action.frame_range.y) + 1):
            bpy.context.scene.frame_set(frame)
            for pose in rig.pose.bones:
                assert all(abs(v - 1) < 2e-5 for v in pose.scale), (action.name, frame, pose.name, tuple(pose.scale))
            assert rig.pose.bones["Root"].location.length < 1e-6
    print(f"EDUHUMAN_VALIDATION bones=22 vertices={len(body.data.vertices)} polygons={len(body.data.polygons)} materials={len(body.data.materials)} weights=normalized external_images=0")


def export(rig, body, actions):
    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    body.select_set(True)
    bpy.context.view_layer.objects.active = rig
    for clip, action in actions.items():
        rig.animation_data.action = action
        # Single-action FBX takes use the scene name, not the action name.
        bpy.context.scene.name = clip
        bpy.context.scene.frame_start = action["export_start"]
        bpy.context.scene.frame_end = action["export_end"]
        bpy.context.scene.frame_set(0)
        destination = OUTPUT / f"EduHuman_{clip}.fbx"
        bpy.ops.export_scene.fbx(
            filepath=str(destination), use_selection=True,
            object_types={"ARMATURE", "MESH"},
            apply_unit_scale=True, apply_scale_options="FBX_SCALE_ALL",
            axis_forward="-Z", axis_up="Y", global_scale=1.0,
            bake_space_transform=True,
            use_mesh_modifiers=True, mesh_smooth_type="FACE",
            use_armature_deform_only=False, add_leaf_bones=False,
            bake_anim=True, bake_anim_use_all_bones=True,
            bake_anim_use_nla_strips=False, bake_anim_use_all_actions=False,
            bake_anim_force_startend_keying=True, bake_anim_step=1.0,
            bake_anim_simplify_factor=0.0, path_mode="AUTO",
            embed_textures=False, use_custom_props=True,
        )
        print(f"EDUHUMAN_EXPORT {destination.name} action={clip} frames=({action['export_start']}, {action['export_end']}) seconds={action['duration_seconds']:.3f}")


def add_documentation():
    guide = bpy.data.texts.new("START HERE - EduHuman")
    guide.write("""EDUHUMAN / EDITABLE EDUCATIONAL HUMANOID

Original mathematical geometry and keyframes created with AI coding assistance.
No external model, character texture, rig or motion was used as an input.
This describes the production process; it does not assert exclusive copyright
ownership or claim that a human manually authored every element.

SOURCE: tools/generate_eduhuman.py
BODY: EduHumanBody, six plain-color materials, no image dependencies.
RIG: EduHuman, 22 humanoid bones, scale 1. Blender Z-up / -Y-forward.
HEIGHT: about 1.8 metres. Rest pose feet at the ground plane.
WEIGHTS: one explicit normalized influence per vertex, grouped by body part.
The mechanical joints make this rigid weighting easy to understand and edit.
Use Edit Mode > vertex group > Select to edit an individual body region.

ACTIONS: Idle (2.4 seconds); Jog (0.8 seconds, looping); Kick (1.6 seconds).
Kick uses the right leg. Root is fixed in every action (in-place movement).
Each FBX contains the same complete mesh/rig and only one baked action.
Frame 0 is a common rest/bind reference, excluded from each exported clip.
The Blender file keeps all three actions for editing in the Action Editor.
The preview collection is excluded from all FBX exports.
""")


def preview(rig, body, actions, destination):
    """Render newly generated 3D geometry; no image manipulation or inputs."""
    scene = bpy.context.scene
    collection = bpy.data.collections.new("Preview only - excluded from FBX")
    scene.collection.children.link(collection)

    def move_to_preview(obj):
        for old in list(obj.users_collection):
            old.objects.unlink(obj)
        collection.objects.link(obj)

    rig.animation_data.action = actions["Idle"]
    scene.frame_set(1)
    rig.location.x = -.80
    jog_rig = rig.copy()
    jog_rig.data = rig.data.copy()
    collection.objects.link(jog_rig)
    jog_rig.location = (.03, .10, 0)
    jog_rig.animation_data.action = actions["Jog"]
    jog_body = body.copy()
    collection.objects.link(jog_body)
    jog_body.parent = jog_rig
    jog_body.modifiers[0].object = jog_rig
    kick_rig = rig.copy()
    kick_rig.data = rig.data.copy()
    collection.objects.link(kick_rig)
    kick_rig.location = (.90, .18, 0)
    kick_rig.animation_data.action = actions["Kick"]
    kick_body = body.copy()
    collection.objects.link(kick_body)
    kick_body.parent = kick_rig
    kick_body.modifiers[0].object = kick_rig
    scene.frame_set(23)
    # Freeze preview copies so the saved editable rig remains on Idle frame 1.
    for duplicate in (jog_rig, kick_rig):
        poses = {p.name: p.matrix.copy() for p in duplicate.pose.bones}
        duplicate.animation_data_clear()
        for name, matrix in poses.items():
            duplicate.pose.bones[name].matrix = matrix
    scene.frame_set(1)
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -.006))
    ground = bpy.context.object
    ground.name = "Preview floor"
    mat = bpy.data.materials.new("Preview floor graphite")
    mat.diffuse_color = (.055, .077, .102, 1)
    ground.data.materials.append(mat)
    move_to_preview(ground)
    for name, location, energy, size in [
        ("Soft key", (1.5, -3.0, 4.5), 400, 4),
        ("Cool fill", (-3.0, -1, 2.8), 200, 3),
        ("Rim", (.2, 3, 3.0), 600, 3),
    ]:
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.shape, data.size = energy, "DISK", size
        obj = bpy.data.objects.new(name, data)
        collection.objects.link(obj)
        obj.location = location
        obj.rotation_euler = (Vector((0, 0, .9)) - obj.location).to_track_quat("-Z", "Y").to_euler()
    data = bpy.data.cameras.new("Preview camera")
    camera = bpy.data.objects.new("Preview camera", data)
    collection.objects.link(camera)
    camera.location = (3.6, -7.0, 3.0)
    camera.rotation_euler = (Vector((0, -.03, .90)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    data.type, data.ortho_scale = "ORTHO", 3.90
    scene.camera = camera
    if scene.world is None:
        scene.world = bpy.data.worlds.new("EduHuman preview world")
    scene.world.color = (.12, .12, .12)
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 32
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 1500, 950
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "AgX"
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(destination)
    bpy.ops.render.render(write_still=True)
    rig.location = (0, 0, 0)
    # Keep only the editable source character in the .blend; preview is reproducible.
    for obj in list(collection.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.collections.remove(collection)
    scene.camera = None


def main():
    # Discard the user's startup file, including hidden scenes and fake-user data.
    bpy.ops.wm.read_factory_settings(use_empty=True)
    # A second main() call in Blender's Python console must also start fresh.
    BONE_SPECS.clear()
    PARTS.clear()
    MATERIALS.clear()
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-preview", action="store_true")
    options = parser.parse_args(args)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    bpy.context.preferences.filepaths.save_version = 0
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.fps = FPS
    rig = create_rig()
    body = create_character(rig)
    actions = {clip: create_action(rig, clip, duration) for clip, duration in [("Idle", 2.4), ("Jog", .8), ("Kick", 1.6)]}
    validate(rig, body, actions)
    export(rig, body, actions)
    add_documentation()
    if not options.no_preview:
        destination = ROOT / "x64" / "eduhuman-preview.png"
        destination.parent.mkdir(parents=True, exist_ok=True)
        preview(rig, body, actions, destination)
    rig.animation_data.action = actions["Idle"]
    scene.name = "EduHuman Editing"
    scene.frame_start, scene.frame_end = 1, 73
    scene.frame_set(1)
    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    body.select_set(True)
    bpy.context.view_layer.objects.active = rig
    # Drop unused construction meshes and the optional preview's datablocks.
    # The three editable actions survive because they explicitly use fake users.
    bpy.data.orphans_purge(do_recursive=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "EduHuman.blend"))
    print("EDUHUMAN_COMPLETE")


if __name__ == "__main__":
    main()
