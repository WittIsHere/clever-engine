#include "c_Transform.h"
#include "GameObject.h"
#include "JSONParser.h"

c_Transform::c_Transform(GameObject* parent, ComponentData* data) : Component(parent, data)
{
	this->type = COMPONENT_TYPE::TRANSFORM;
	transformData = (TransformData*)data;
	UpdateLocalTransform();
}

c_Transform::~c_Transform()
{
}

bool c_Transform::Enable()
{
	return true;
}

bool c_Transform::Update()
{
	return true;
}

bool c_Transform::Disable()
{
	return true;
}

bool c_Transform::SaveState(ParsonNode& root) const
{
	root.SetNumber("Type", (uint)type);

	return true;
}

bool c_Transform::LoadState(ParsonNode& root)
{
	return true;
}

void c_Transform::SetLocalPosition(const float3& newPosition)
{
	transformData->position = newPosition;

	UpdateLocalTransform();
}

void c_Transform::SetLocalRotation(const Quat& newRotation)
{
	transformData->rotation = newRotation;

	UpdateLocalTransform();
}

void c_Transform::SetLocalRotation(const float3& newRotation)
{
	transformData->rotation = Quat::FromEulerXYZ(newRotation.x, newRotation.y, newRotation.z);

	UpdateLocalTransform();
}

void c_Transform::SetLocalScale(const float3& newScale)
{
	transformData->scale = newScale;

	UpdateLocalTransform();
}

float3 c_Transform::GetLocalPosition() const
{
	return transformData->position;
}

Quat c_Transform::GetLocalRotation() const
{
	return transformData->rotation;
}

float3 c_Transform::GetLocalEulerRotation() const
{
	return transformData->rotation.ToEulerXYZ();
}

float3 c_Transform::GetLocalScale() const
{
	return transformData->scale;
}

float* c_Transform::GetLocalTransform() const
{
	return (float*)localTransform.ptr();
}

void c_Transform::UpdateLocalTransform()
{
	localTransform = float4x4::FromTRS(transformData->position, transformData->rotation, transformData->scale);
	UpdateWorldTransform();
	updateWorld = true;
}

float4x4 c_Transform::GetWorldTransform()
{
	if (updateWorld == true)
		UpdateWorldTransform();
		
	return worldTransform;
}

float* c_Transform::GetWorldTransformPtr()
{
	if (updateWorld == true)
		UpdateWorldTransform();

	return (float*)worldTransform.Transposed().ptr();
}

void c_Transform::UpdateWorldTransform()
{
	//get owner
	GameObject* owner = this->GetOwner();

	if (owner->GetParent() != nullptr)
	{
		if (owner->GetParent()->isRoot == false)	//if the owner has a parent that is not the root, we multiply the world transform (that will be its local if it is a root) by the local transform of the owner
		{
			float4x4 parentTransform = owner->GetParent()->GetComponentTransform()->GetWorldTransform();
			worldTransform = parentTransform * localTransform;
		}
		else	//if the owner has the root as parent, this GO is the parent node and its localTransform is the global
			worldTransform = localTransform;
	}
	
	if (owner->GetChildCount() > 0) //if the world transform of the parent changes, there has to be a way of telling the children to update their own
	{
		for (int i = 0; i < owner->GetChildCount(); i++)
		{
			GameObject* child = owner->GetChildData(i);
			child->GetComponentTransform()->updateWorld = true;
		}
	}
	updateWorld = false; 
}

