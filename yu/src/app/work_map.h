
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
	WorkItem* endWork;
	WorkItem* testRenderer;
};
extern WorkMap* gWorkMap;

void Clear(WorkMap* workMap);
void SubmitWork(WorkMap* workMap);

void InitWorkMap();
void FreeWorkMap();

}