#include "EffectBoxController.h"
#include "math.h"
#include "EffectBox.h"
#include "Scene.h"
#include "Extension.h"
#include "Vector3.h"
using namespace MyEngine;
EffectBoxController::EffectBoxController(void)
{
}


EffectBoxController::~EffectBoxController(void)
{

}
void EffectBoxController::UpdateActor(MyEngine::Actor & actor , bool & isDone,double delta)
{	

	Vector3 rotation;
	rotation.setValue(actor.getRotation()->getX()+0.13f,actor.getRotation()->getY()-0.01f,actor.getRotation()->getZ()-0.01f);
	actor.setRotation(rotation);


}