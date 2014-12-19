
namespace yu
{
struct WorkItem;

struct FrameWorkItemResult
{
	WorkItem* frameStartItem;
	WorkItem* frameEndItem;
};

struct WorkMap
{
//pre-defined work
	WorkItem* startWork;
	WorkItem* inputWork;
	WorkItem* cameraControllerWork;
	WorkItem* testRenderer;
	WorkItem* endWork;
};
extern WorkMap* gWorkMap;

void Clear(WorkMap* workMap);
void SubmitWork(WorkMap* workMap);

void InitWorkMap();
void FreeWorkMap();

}