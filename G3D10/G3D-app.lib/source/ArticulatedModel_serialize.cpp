/**
  \file G3D-app.lib/source/ArticulatedModel_serialize.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#include "G3D-app/ArticulatedModel.h"
#include "G3D-base/XML.h"

namespace G3D {

ArticulatedModel::CleanGeometrySettings::CleanGeometrySettings(const Any& a) {
    *this = CleanGeometrySettings();
    AnyTableReader r(a);
    r.getIfPresent("forceVertexMerging",   forceVertexMerging);
    r.getIfPresent("allowVertexMerging",   allowVertexMerging);
    r.getIfPresent("forceComputeNormals",  forceComputeNormals);
    r.getIfPresent("forceComputeTangents", forceComputeTangents);

    float f;
    if (r.getIfPresent("maxNormalWeldAngleDegrees", f)) {
        maxNormalWeldAngle = toRadians(f);
    }
    if (r.getIfPresent("maxSmoothAngleDegrees", f)) {
        maxSmoothAngle = toRadians(f);
    }
    r.getIfPresent("maxEdgeLength", maxEdgeLength);
    r.verifyDone();
}


Any ArticulatedModel::CleanGeometrySettings::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::CleanGeometrySettings");
    a["forceVertexMerging"]         = forceVertexMerging;
    a["allowVertexMerging"]         = allowVertexMerging;
    a["forceComputeNormals"]        = forceComputeNormals;
    a["forceComputeTangents"]       = forceComputeTangents;
    a["maxNormalWeldAngleDegrees"]  = toDegrees(maxNormalWeldAngle);
    a["maxSmoothAngleDegrees"]      = toDegrees(maxSmoothAngle);
    a["maxEdgeLength"]              = maxEdgeLength;
    return a;
}

//////////////////////////////////////////////////////////////////////

ArticulatedModel::Specification::Specification(const Any& a) {
    *this = Specification();

    if (a.type() == Any::STRING) {
        try {
            filename = a.resolveStringAsFilename();
        } catch (const FileNotFound& e) {
            throw ParseError(a.source().filename, a.source().line, e.message);
        }

        if (endsWith(filename, ".ArticulatedModel.Any")) {
            *this = Specification(Any::fromFile(filename));
        }

    } else {

        AnyTableReader r(a);
        Any f;
        if (! r.getIfPresent("filename", f)) {
            a.verify(false, "Expected a filename field in ArticulatedModel::Specification");
        }
        f.verifyType(Any::STRING);
        try {
            filename = f.resolveStringAsFilename();   
        } catch (const FileNotFound& e) {
            throw ParseError(f.source().filename, f.source().line, e.message);
        }

        r.getIfPresent("stripMaterials",            stripMaterials);
        r.getIfPresent("stripVertexColors",         stripVertexColors);
        r.getIfPresent("stripLightMaps",            stripLightMaps);
        r.getIfPresent("stripLightMapCoords",       stripLightMapCoords);
        r.getIfPresent("alphaFilter",               alphaFilter);
        r.getIfPresent("refractionHint",            refractionHint);
        r.getIfPresent("invertPrecomputedNormalYAxis",invertPrecomputedNormalYAxis);

        Any temp;
        if (r.getIfPresent("meshMergeOpaqueClusterRadius", temp)) {
            meshMergeOpaqueClusterRadius = anyToMeshMergeRadius(temp);
        }

        if (r.getIfPresent("meshMergeTransmissiveClusterRadius", temp)) {
            meshMergeTransmissiveClusterRadius = anyToMeshMergeRadius(temp);
        }

        r.getIfPresent("cleanGeometrySettings",     cleanGeometrySettings);
        r.getIfPresent("scale",                     scale);
        r.getIfPresent("preprocess",                preprocess);
        r.getIfPresent("cachable",                  cachable);  

        r.getIfPresent("objOptions",                objOptions);
        r.getIfPresent("heightfieldOptions",        heightfieldOptions);
        r.getIfPresent("hairOptions",               hairOptions);
        r.getIfPresent("colladaOptions",            colladaOptions);

        r.getIfPresent("voxelOptions",              voxelOptions);

        r.verifyDone();
    }
}


size_t ArticulatedModel::Specification::hashCode() const {
    return 
        HashTrait<String>().hashCode(filename) ^ 
        (size_t)(stripMaterials) ^
        (alphaFilter.hashCode() << 6) ^
        (refractionHint.hashCode() << 7) ^
        hairOptions.hashCode() ^
        ((size_t)(stripLightMaps) << 3) ^
        ((size_t)(stripLightMapCoords) << 4) ^
        ((size_t)(scale * 100)) ^
        (size_t)(voxelOptions.hashCode());
}


Any ArticulatedModel::Specification::toAny() const {
    Any a(Any::TABLE, "ArticulatedModel::Specification");
    a["filename"]                  = filename;
    a["stripMaterials"]            = stripMaterials;
    a["stripVertexColors"]         = stripVertexColors;
    a["stripLightMaps"]            = stripLightMaps;
    a["stripLightMapCoords"]       = stripLightMapCoords;
    a["alphaFilter"]               = alphaFilter;
    a["refractionHint"]            = refractionHint;
    a["meshMergeOpaqueClusterRadius"] = meshMergeRadiusToAny(meshMergeOpaqueClusterRadius);
    a["meshMergeTransmissiveClusterRadius"] = meshMergeRadiusToAny(meshMergeTransmissiveClusterRadius);
    a["cleanGeometrySettings"]     = cleanGeometrySettings;
    a["scale"]                     = scale;
    a["objOptions"]                = objOptions;
    a["heightfieldOptions"]        = heightfieldOptions;
    a["hairOptions"]               = hairOptions;
    a["cachable"]                  = cachable;
    a["colladaOptions"]            = colladaOptions;
    a["voxelOptions"]              = voxelOptions;

    if (preprocess.size() > 0) {
        a["preprocess"] = Any(preprocess, "preprocess");
    }
    return a;
}


bool ArticulatedModel::Specification::operator==(const Specification& other) const {
    if ((filename == other.filename) &&
        (stripMaterials == other.stripMaterials) &&
        (stripVertexColors == other.stripVertexColors) &&
        (stripLightMaps == other.stripLightMaps) &&
        (alphaFilter == other.alphaFilter) &&
        (refractionHint == other.refractionHint) &&
        (stripLightMapCoords == other.stripLightMapCoords) &&
        (meshMergeOpaqueClusterRadius == other.meshMergeOpaqueClusterRadius) &&
        (meshMergeTransmissiveClusterRadius == other.meshMergeTransmissiveClusterRadius) &&
        (scale == other.scale) &&
        (cleanGeometrySettings == other.cleanGeometrySettings) &&
        (cachable == other.cachable) &&
        (objOptions == other.objOptions) &&
        (hairOptions == other.hairOptions) &&
        (heightfieldOptions == other.heightfieldOptions) &&
        (hairOptions == other.hairOptions) &&
        (colladaOptions == other.colladaOptions) &&
        (voxelOptions == other.voxelOptions) &&
        (preprocess.size() == other.preprocess.size())) {
        // Compare preprocess instructions
        for (int i = 0; i < preprocess.size(); ++i) {
            if (preprocess[i] != other.preprocess[i]) {
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}


ArticulatedModel::Specification::ColladaOptions::ColladaOptions(const Any& a) {
    *this = ColladaOptions();
    a.verifyName("ColladaOptions");
    AnyTableReader r(a);
    String s;
    if (r.getIfPresent("transmissiveChoice", s)) {
        transmissiveChoice = TransmissiveOption(toUpper(s));
    }
}


Any ArticulatedModel::Specification::ColladaOptions::toAny() const {
    Any a(Any::TABLE, "ColladaOptions");
    a["transmissiveChoice"] = transmissiveChoice.toString();
    return a;
}


ArticulatedModel::Specification::HeightfieldOptions::HeightfieldOptions(const Any& a) {
    *this = HeightfieldOptions();
    a.verifyName("HeightfieldOptions");
    AnyTableReader r(a);
    r.getIfPresent("textureScale", textureScale);
    r.getIfPresent("generateBackfaces", generateBackfaces);
    r.getIfPresent("elevationScale", elevationScale);
}


Any ArticulatedModel::Specification::HeightfieldOptions::toAny() const {
    Any a(Any::TABLE, "HeightfieldOptions");
    a["textureScale"] = textureScale;
    a["generateBackfaces"] = generateBackfaces;
    a["elevationScale"] = elevationScale;
    return a;
}


ArticulatedModel::Specification::HairOptions::HairOptions(const Any& a) {
    *this = HairOptions();
    a.verifyName("HairOptions");
    AnyTableReader r(a);
    r.getIfPresent("sideCount", sideCount);
    r.getIfPresent("separateSurfacePerStrand", separateSurfacePerStrand);
    r.getIfPresent("strandRadiusMultiplier", strandRadiusMultiplier);
}


Any ArticulatedModel::Specification::HairOptions::toAny() const {
    Any a(Any::TABLE, "HairOptions");
    a["sideCount"]                  = sideCount;
    a["strandRadiusMultiplier"]     = strandRadiusMultiplier;
    a["separateSurfacePerStrand"]   = separateSurfacePerStrand;
    return a;
}


ArticulatedModel::Specification::VoxelOptions::VoxelOptions(const Any& a) {
    *this = VoxelOptions();
    a.verifyName("VoxelOptions");
    AnyTableReader r(a);
    r.getIfPresent("removeInternalVoxels", removeInternalVoxels);
    r.getIfPresent("treatBorderAsOpaque",  treatBorderAsOpaque);
}


Any ArticulatedModel::Specification::VoxelOptions::toAny() const {
    Any a(Any::TABLE, "VoxelOptions");
    a["removeInternalVoxels"]                 = removeInternalVoxels;
    a["treatBorderAsOpaque"]                  = treatBorderAsOpaque;
    return a;
}



//////////////////////////////////////////////////////////////////////

ArticulatedModel::PoseSpline::PoseSpline() {}


ArticulatedModel::PoseSpline::PoseSpline(const Any& any)  {
    for (Any::AnyTable::Iterator it = any.table().begin(); it.isValid(); ++it) {
        partSpline.getCreate(it->key) = it->value;
    }
}

 
void ArticulatedModel::PoseSpline::get(float t, ArticulatedModel::Pose& pose) {
    for (SplineTable::Iterator it = partSpline.begin(); it.isValid(); ++it) {
        const PhysicsFrameSpline& spline = it->value;
        if (spline.control.size() > 0) {
            const PhysicsFrame& p = spline.evaluate(t);
            pose.frameTable.set(it->key, p);
            debugAssert(! p.rotation.isNaN());
        }
    }
}

///////////////////////////////////////////////////////////////////////

ArticulatedModel::Instruction::Identifier::Identifier(const Any& a) {
    switch (a.type()) {

    case Any::STRING:
        name = a.string();
        break;

    case Any::ARRAY: // Intentionally fall-through
    case Any::EMPTY_CONTAINER: 
        a.verifySize(0);
        if (a.name() == "root") {
            *this = root();
        } else if (a.name() == "all") {
            *this = all();
        } else {
            a.verify(false, "Illegal function call: " + a.name());
        }
        break;

    default:
        a.verify(false, "Expected a name, integer ID, root(), or all()");
    }
}

Any ArticulatedModel::Instruction::Identifier::toAny() const {
    if (isAll()) {
        return Any(Any::ARRAY, "all");
    } else if (isRoot()) {
        return Any(Any::ARRAY, "root");
    } else {
        return Any(name);
    }
}

///////////////////////////////////////////////////////////////////////

Any ArticulatedModel::Instruction::toAny() const {
    return source;
}

ArticulatedModel::Instruction::Instruction(const Any& any) {
    any.verifyType(Any::ARRAY);

    source = any;
    part = Identifier();
    mesh = Identifier();
    arg = Any();

    const String& instructionName = any.name();

    if (instructionName == "scale") {

        type = SCALE;
        any.verifySize(1);
        arg = any[0];

    } else if (instructionName == "moveCenterToOrigin") {

        type = MOVE_CENTER_TO_ORIGIN;
        any.verifySize(0);

    } else if (instructionName == "moveBaseToOrigin") {

        type = MOVE_BASE_TO_ORIGIN;
        any.verifySize(0);

    } else if (instructionName == "setCFrame") {

        type = SET_CFRAME;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "transformCFrame") {

        type = TRANSFORM_CFRAME;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "transformGeometry") {

        type = TRANSFORM_GEOMETRY;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "removeMesh") {

        type = REMOVE_MESH;
        any.verifySize(1);
        mesh = any[0];

    } else if (instructionName == "reverseWinding") {

        type = REVERSE_WINDING;
        any.verifySize(1);
        mesh = any[0];

    } else if (instructionName == "removePart") {

        type = REMOVE_PART;
        any.verifySize(1);
        part = any[0];

    } else if (instructionName == "setMaterial") {

        type = SET_MATERIAL;
        any.verifySize(2, 3);
        mesh = any[0];
        arg  = any[1];
        // Parse the third (boolean) argument explicitly
        // during application.

    } else if (instructionName == "setTwoSided") {

        type = SET_TWO_SIDED;
        any.verifySize(2);
        mesh = any[0];
        arg = any[1];

    } else if (instructionName == "mergeAll") {

        type = MERGE_ALL;
        // if the argument is missing, cast to trigger an error now instead of during preprocessing
        (void)anyToMeshMergeRadius(any[0]);
        (void)anyToMeshMergeRadius(any[1]);
        arg = any[0];

    } else if (instructionName == "renamePart") {

        type = RENAME_PART;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else if (instructionName == "renameMesh") {

        type = RENAME_MESH;
        any.verifySize(2);
        mesh = any[0];
        arg = any[1];

    } else if (instructionName == "add") {

        type = ADD;
        mesh = Identifier::none();
        if (any.size() == 2) {
            any.verifySize(2);
            part = any[0];
            arg = any[1];
        } else {
            any.verifySize(1);
            part = Identifier::none();
            arg = any[0];
        }

    } else if (instructionName == "copyTexCoord0ToTexCoord1") {
        type = COPY_TEXCOORD0_TO_TEXCOORD1;
        any.verifySize(1);
        mesh = any[0];
        arg = any[1];
    } else if (instructionName == "scaleAndOffsetTexCoord1") {
        type = SCALE_AND_OFFSET_TEXCOORD1;
        any.verifySize(3);
        mesh = any[0];
        arg = any[1];

    } else if (instructionName == "scaleAndOffsetTexCoord0") {
        type = SCALE_AND_OFFSET_TEXCOORD0;
        any.verifySize(3);
        mesh    = any[0];
        arg     = any[1];

    } else if (instructionName == "intersectBox") {

        type = INTERSECT_BOX;
        any.verifySize(2);
        part = any[0];
        arg = any[1];

    } else {

        any.verify(false, String("Unknown instruction: \"") + instructionName + "\"");

    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////


ArticulatedModel::Pose::Pose(const Any& any) : numInstances(1) {
    if (any.nameBeginsWith("UniversalMaterial") || 
        any.nameBeginsWith("Texture") || 
        any.nameBeginsWith("Color")) {
        // Special case of a single material casting to an entire pose
        materialTable.set("mesh", UniversalMaterial::create(any));
        return;
    }

    AnyTableReader reader(any);

    Table<String, UniversalMaterial::Specification> specTable;
    if (reader.getIfPresent("materialTable", specTable)) {
        for (Table<String, UniversalMaterial::Specification>::Iterator it = specTable.begin(); it.hasMore(); ++it) {
            materialTable.set(it->key, UniversalMaterial::create(it->value));
        }
    }

    reader.getIfPresent("scale", scale);

    reader.getIfPresent("numInstances", numInstances);
    any.verify(numInstances >= 0);

    Any uniformTableAny;
    if (reader.getIfPresent("uniformTable", uniformTableAny)) {
        uniformTable = shared_ptr<UniformTable>(new UniformTable(uniformTableAny));
    }

    reader.getIfPresent("frameTable", frameTable);
    reader.verifyDone();
}


void ArticulatedModel::saveGeometryAsCode(const String& filename, bool compress) {
    TextOutput::Settings settings;
    settings.numColumns = 256;
    TextOutput file(filename, settings);

    const Array<int>& indexArray = meshArray()[0]->cpuIndexArray;
    const Array<CPUVertexArray::Vertex>& vertexArray = meshArray()[0]->geometry->cpuVertexArray.vertex;

    file.writeSymbol("{");
    file.writeNewline();
    file.pushIndent();

    const String& separator = compress ? "," : ", ";

    file.printf("const int numVertices = %d;\n", vertexArray.size());
    file.printf("const float position[][3] = {");
    file.pushIndent();
    for (int i = 0; i < vertexArray.size(); ++i) {
        const Point3& v = vertexArray[i].position;
        file.writeCNumber(v.x, false, compress);
        file.printf(separator);
        file.writeCNumber(v.y, false, compress);
        file.printf(separator);
        file.writeCNumber(v.z, false, compress);
        if (i < vertexArray.size() - 1) {
            file.printf(separator);
            if (compress && (file.column() > 200)) {
                file.writeNewline();
            }
        }
    }
    file.printf("};");
    file.popIndent();
    file.writeNewline();

    file.printf("const float normal[][3] = {");
    file.pushIndent();
    for (int i = 0; i < vertexArray.size(); ++i) {
        const Vector3& n = vertexArray[i].normal;
        file.writeCNumber(n.x, false, compress);
        file.printf(separator);
        file.writeCNumber(n.y, false, compress);
        file.printf(separator);
        file.writeCNumber(n.z, false, compress);
        if (i < vertexArray.size() - 1) {
            file.printf(separator);
            if (compress && (file.column() > 200)) {
                file.writeNewline();
            }
        }
    }
    file.printf("};");
    file.popIndent();
    file.writeNewline();
    file.printf("const float tangent[][4] = {");
    file.pushIndent();
    for (int i = 0; i < vertexArray.size(); ++i) {
        const Vector4& t = vertexArray[i].tangent;
        file.writeCNumber(t.x, false, compress);
        file.printf(separator);
        file.writeCNumber(t.y, false, compress);
        file.printf(separator);
        file.writeCNumber(t.z, false, compress);
        file.printf(separator);
        file.writeCNumber(t.w, false, compress);
        if (i < vertexArray.size() - 1) {
            file.printf(separator);
            if (compress && (file.column() > 200)) {
                file.writeNewline();
            }
        }
    }
    file.printf("};");
    file.popIndent();
    file.writeNewline();
    file.printf("const float texCoord[][2] = {");
    file.pushIndent();
    for (int i = 0; i < vertexArray.size(); ++i) {
        const Point2& t = vertexArray[i].texCoord0;
        file.writeCNumber(t.x, false, compress);
        file.printf(separator);
        file.writeCNumber(t.y, false, compress);
        if (i < vertexArray.size() - 1) {
            file.printf(separator);
            if (compress && (file.column() > 200)) {
                file.writeNewline();
            }
        }
    }
    file.printf("};");
    file.popIndent();
    file.writeNewline();
    file.printf("const int index[] = {");
    file.pushIndent();
    for (int i = 0; i < indexArray.size(); ++i) {
        file.writeCNumber(indexArray[i], false, compress);
        if (i < indexArray.size() - 1) {
            file.printf(separator);
            if (compress && (file.column() > 200)) {
                file.writeNewline();
            }
        }
    }
    file.printf("};");
    file.popIndent();
    file.writeNewline();
    file.printf("const int numIndices = %d;\n", indexArray.size());
    file.popIndent();
    file.writeSymbol("}");
    file.writeNewline();

    file.commit();
}


} // namespace G3D
