## spine-cpp修改记录

原仓库：https://github.com/EsotericSoftware/spine-runtimes

### spine-cpp-42

无修改

### spine-cpp-41

在spine-cpp-42所做修改的基础上

- 复制spine-cpp-42的`BlockAllocator.h`,`SkeletonRenderer.h`,`SkeletonRenderer.cpp`

- 在`spine.h`中添加`SkeletonRenderer.h`头文件：

```diff
@@ -87,6 +87,7 @@
 #include <spine/SkeletonClipping.h>
 #include <spine/SkeletonData.h>
 #include <spine/SkeletonJson.h>
+#include <spine/SkeletonRenderer.h>
 #include <spine/Skin.h>
 #include <spine/Slot.h>
 #include <spine/SlotData.h>
```

### spine-cpp-40

在spine-cpp-41所做修改的基础上

- 对`SkeletonRenderer.h`做出如下修改：

```diff
@@ -30,6 +30,7 @@
 #ifndef Spine_SkeletonRenderer_h
 #define Spine_SkeletonRenderer_h
 
+#include <spine/Atlas.h>
 #include <spine/BlockAllocator.h>
 #include <spine/BlendMode.h>
 #include <spine/SkeletonClipping.h>
```

- 对`SkeletonRenderer.cpp`做出如下修改：

```diff
@@ -177,12 +177,12 @@
 			}
 
 			worldVertices->setSize(8, 0);
-			regionAttachment->computeWorldVertices(slot, *worldVertices, 0, 2);
+			regionAttachment->computeWorldVertices(slot.getBone(), *worldVertices, 0, 2);
 			verticesCount = 4;
 			uvs = &regionAttachment->getUVs();
 			indices = quadIndices;
 			indicesCount = 6;
-			texture = regionAttachment->getRegion()->rendererObject;
+			texture = ((AtlasRegion *) regionAttachment->getRendererObject())->page->getRendererObject();
 
 		} else if (attachment->getRTTI().isExactly(MeshAttachment::rtti)) {
 			MeshAttachment *mesh = (MeshAttachment *) attachment;
@@ -200,7 +200,7 @@
 			uvs = &mesh->getUVs();
 			indices = &mesh->getTriangles();
 			indicesCount = (int32_t) indices->size();
-			texture = mesh->getRegion()->rendererObject;
+			texture = ((AtlasRegion *) mesh->getRendererObject())->page->getRendererObject();
 
 		} else if (attachment->getRTTI().isExactly(ClippingAttachment::rtti)) {
 			ClippingAttachment *clip = (ClippingAttachment *) slot.getAttachment();
```

### spine-cpp-38

在spine-cpp-40所做修改的基础上无修改

不，还是有修改的，对官方`SkeletonJson.cpp`和`SkeletonBinary.cpp`做出如下修改以解除3.8.75（网上广泛流传的破解版本）版本限制:joy: ：

```diff
@@ -128,13 +128,6 @@
 
 	char *skeletonData_version = readString(input);
 	skeletonData->_version.own(skeletonData_version);
-    if ("3.8.75" == skeletonData->_version) {
-        delete input;
-        delete skeletonData;
-        setError("Unsupported skeleton data, please export with a newer version of Spine.", "");
-        return NULL;
-    }
-
 	skeletonData->_x = readFloat(input);
 	skeletonData->_y = readFloat(input);
 	skeletonData->_width = readFloat(input);

```

```diff
@@ -133,11 +133,6 @@
 	if (skeleton) {
 		skeletonData->_hash = Json::getString(skeleton, "hash", 0);
 		skeletonData->_version = Json::getString(skeleton, "spine", 0);
-		if ("3.8.75" == skeletonData->_version) {
-            delete skeletonData;
-            setError(root, "Unsupported skeleton data, please export with a newer version of Spine.", "");
-            return NULL;
-        }
 		skeletonData->_x = Json::getFloat(skeleton, "x", 0);
 		skeletonData->_y = Json::getFloat(skeleton, "y", 0);
 		skeletonData->_width = Json::getFloat(skeleton, "width", 0);

```

### spine-cpp-37

在spine-cpp-38所做修改的基础上

- 对`SkeletonRenderer.cpp`再做出如下修改：

```diff
@@ -151,7 +151,7 @@
 		}
 
 		// Early out if the slot color is 0 or the bone is not active
-		if ((slot.getColor().a == 0 || !slot.getBone().isActive()) && !attachment->getRTTI().isExactly(ClippingAttachment::rtti)) {
+		if ((slot.getColor().a == 0) && !attachment->getRTTI().isExactly(ClippingAttachment::rtti)) {
 			clipper.clipEnd(slot);
 			continue;
 		}
```

