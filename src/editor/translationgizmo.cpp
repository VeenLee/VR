#include "translationgizmo.h"
#include <QOpenGLFunctions_3_2_Core>
#include <qopenglshaderprogram.h>

#include "irisgl/src/math/intersectionhelper.h"
#include "irisgl/src/math/mathhelper.h"
#include "irisgl/src/core/irisutils.h"
#include "irisgl/src/scenegraph/scene.h"
#include "irisgl/src/scenegraph/cameranode.h"
#include "irisgl/src/graphics/graphicshelper.h"
#include "uimanager.h"
#include "../commands/transfrormscenenodecommand.h"
#include "irisgl/src/math/mathhelper.h"

TranslationHandle::TranslationHandle(Gizmo* gizmo, GizmoAxis axis)
{
	this->gizmo = gizmo;

	switch (axis) {
	case GizmoAxis::X:
		handleExtent = QVector3D(1, 0, 0);
		planes.append(QVector3D(0, 1, 0));
		planes.append(QVector3D(0, 0, 1));
		setHandleColor(QColor(255, 0, 0));
		break;
	case GizmoAxis::Y:
		handleExtent = QVector3D(0, 1, 0);
		planes.append(QVector3D(1, 0, 0));
		planes.append(QVector3D(0, 0, 1));
		setHandleColor(QColor(0, 255, 0));
		break;
	case GizmoAxis::Z:
		handleExtent = QVector3D(0, 0, 1);
		planes.append(QVector3D(1, 0, 0));
		planes.append(QVector3D(0, 1, 0));
		setHandleColor(QColor(0, 0, 255));
		break;
	}
}

bool TranslationHandle::isHit(QVector3D rayPos, QVector3D rayDir)
{
    auto gizmoTrans = gizmo->getTransform();

    // calculate world space position of the segment representing the handle
    auto p1 = gizmoTrans * QVector3D(0,0,0);
    auto q1 = gizmoTrans * (handleExtent * handleLength * gizmo->getGizmoScale() * handleScale);

    auto p2 = rayPos;
    auto q2 = rayPos + rayDir * 1000;

    float s,t;
    QVector3D c1, c2;
    auto dist = iris::MathHelper::closestPointBetweenSegments(p1, q1, p2, q2, s,t,c1,c2);
    if (dist < handleScale * gizmo->getGizmoScale() * handleScale) {
        return true;
    }

    return false;
}

//todo: transform rayPos and rayDir to gizmo space first then back to world space
// The rayPos and rayDir are in world space, convert them to gizmo space to do calculations then calculate them back
// to world space when we're done
QVector3D TranslationHandle::getHitPos(QVector3D rayPos, QVector3D rayDir)
{
	bool hit = false;
	QVector3D finalHitPos;
	float closestDist = 10000000;

	// transform rayPos and rayDir to gizmo space
	auto gizmoTransform = gizmo->getTransform();
	auto worldToGizmo = gizmoTransform.inverted();
	rayPos = worldToGizmo * rayPos;
	rayDir = QQuaternion::fromRotationMatrix(worldToGizmo.normalMatrix()).rotatedVector(rayDir);

	// loop thru planes
	for (auto normal : planes) {
		float t;
		QVector3D hitPos;

		// flip normal so its facing the ray source
		if (QVector3D::dotProduct(normal, rayPos) < 0)
			normal = -normal;

		if (iris::IntersectionHelper::intersectSegmentPlane(rayPos, rayPos + rayDir * 10000000, iris::Plane(normal, 0), t, hitPos)) {
			// ignore planes at grazing angles
			if (qAbs(QVector3D::dotProduct(rayDir, normal)) < 0.1f)
				continue;
			
			auto hitResult = handleExtent * hitPos;

			// this isnt the first hit, but if it's the closest one then use it
			if (hit) {
				if (rayPos.distanceToPoint(hitResult) < closestDist) {
					finalHitPos = hitResult;
					closestDist = rayPos.distanceToPoint(hitResult);
				}
			}
			else {
				// first hit, just assign it
				finalHitPos = hitResult;
				closestDist = rayPos.distanceToPoint(hitResult);
			}

			hit = true;
		}
	}

	if (hit) {
		//qDebug() << "hit pos: " << finalHitPos;
	}
	else {
		//qDebug() << "no hit";
		// no hit so move to max distance in view direction
		float dominantExtent = iris::MathHelper::sign( QVector3D::dotProduct(rayDir.normalized(), handleExtent));// results in -1 or 1
		finalHitPos = dominantExtent * handleExtent * 10000;
	}

	// now convert it back to world space
	finalHitPos = gizmoTransform * finalHitPos;

	return finalHitPos;
}

TranslationGizmo::TranslationGizmo() :
	Gizmo()
{
	handles[0] = new TranslationHandle(this, GizmoAxis::X);
	handles[1] = new TranslationHandle(this, GizmoAxis::Y);
	handles[2] = new TranslationHandle(this, GizmoAxis::Z);

	loadAssets();
	//handle->setHandleColor(QColor(255, 255, 255));

	dragging = false;
	draggedHandle = nullptr;
}

void TranslationGizmo::loadAssets()
{
	handleMeshes.append(iris::Mesh::loadMesh(IrisUtils::getAbsoluteAssetPath("app/models/axis_x.obj")));
	handleMeshes.append(iris::Mesh::loadMesh(IrisUtils::getAbsoluteAssetPath("app/models/axis_y.obj")));
	handleMeshes.append(iris::Mesh::loadMesh(IrisUtils::getAbsoluteAssetPath("app/models/axis_z.obj")));

	shader = iris::GraphicsHelper::loadShader(
		IrisUtils::getAbsoluteAssetPath("app/shaders/gizmo.vert"),
		IrisUtils::getAbsoluteAssetPath("app/shaders/gizmo.frag"));
}

bool TranslationGizmo::isDragging()
{
	return dragging;
}

void TranslationGizmo::startDragging(QVector3D rayPos, QVector3D rayDir)
{
	//qDebug() << "drag starting";
	draggedHandle = getHitHandle(rayPos, rayDir, hitPos);
	if (draggedHandle == nullptr) {
		dragging = false; // end dragging if no handle was actually hit
		return;
	}

	nodeStartPos = selectedNode->getGlobalPosition();
	dragging = true;
}

void TranslationGizmo::endDragging()
{
	dragging = false;
}

void TranslationGizmo::drag(QVector3D rayPos, QVector3D rayDir)
{
	//qDebug() << "dragging";
	if (draggedHandle == nullptr) {
		//dragging = false;
		return;
	}
	//qDebug()<<"sliding";
	auto slidingPos = draggedHandle->getHitPos(rayPos, rayDir);

	// move node along line
	// do snapping here as well
	auto diff = slidingPos - hitPos;
	
	// apply diff in global space
	auto targetPos = nodeStartPos + diff;
	// bring to local space
	auto localTarget = selectedNode->parent->getGlobalTransform().inverted() * targetPos;
	selectedNode->setLocalPos(localTarget);
	//selectedNode->setLocalPos(nodeStartPos + diff);
}

bool TranslationGizmo::isHit(QVector3D rayPos, QVector3D rayDir)
{
	for (auto i = 0; i< 3; i++) {
		if (handles[i]->isHit(rayPos, rayDir))
		{
			//handle->setHandleColor(QColor(255, 255, 255));
			return true;
		}
	}

	return false;
}

// returns hit position of the hit handle
TranslationHandle* TranslationGizmo::getHitHandle(QVector3D rayPos, QVector3D rayDir, QVector3D& hitPos)
{
	TranslationHandle* closestHandle = nullptr;
	float closestDistance = 10000000;

	for (auto i = 0; i<3; i++)
	{
		if (handles[i]->isHit(rayPos, rayDir)) {
			auto hit = handles[i]->getHitPos(rayPos, rayDir);// bad, move hitPos to ref variable
			auto dist = hitPos.distanceToPoint(rayPos);
			if (dist < closestDistance) {
				closestHandle = handles[i];
				closestDistance = dist;
				hitPos = hit;
			}
		}
	}

	return closestHandle;
}

void TranslationGizmo::render(QOpenGLFunctions_3_2_Core* gl, QVector3D rayPos, QVector3D rayDir, QMatrix4x4& viewMatrix, QMatrix4x4& projMatrix)
{
	gl->glClear(GL_DEPTH_BUFFER_BIT);
	//gl->glDisable(GL_DEPTH_TEST);
	shader->bind();

	shader->setUniformValue("u_viewMatrix", viewMatrix);
	shader->setUniformValue("u_projMatrix", projMatrix);
	shader->setUniformValue("showHalf", false);

	if (dragging) {
		for (int i = 0; i < 3; i++) {
			if (handles[i] == draggedHandle) {
				auto transform = this->getTransform();
				transform.scale(getGizmoScale() * handles[i]->handleRadius);
				shader->setUniformValue("color", QColor(255, 255, 0));
				shader->setUniformValue("u_worldMatrix", transform);
				//handles[i]->draw(gl, shader);
				handleMeshes[i]->draw(gl, shader);
			}
		}
	}
	else {
		QVector3D hitPos;
		auto hitHandle = getHitHandle(rayPos, rayDir, hitPos);

		for (int i = 0; i < 3; i++) {
			for (int i = 0; i < 3; i++) {
				auto transform = this->getTransform();
				transform.scale(getGizmoScale() * handles[i]->handleRadius);
				shader->setUniformValue("color", handles[i]->getHandleColor());
				shader->setUniformValue("u_worldMatrix", transform);

				if (handles[i] == hitHandle)
					shader->setUniformValue("color", QColor(255, 255, 0));
				else
					shader->setUniformValue("color", handles[i]->getHandleColor());
				handleMeshes[i]->draw(gl, shader);
			}
		}
	}

	shader->release();
}