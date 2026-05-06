#pragma once
struct SVM_HOST_STATE
{
};
struct SVM_CONTROLS
{
};
struct VMCB_CACHE
{
	SVM_HOST_STATE host_state;
	SVM_CONTROLS ctrls;
};