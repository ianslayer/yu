
namespace yu
{
struct WorkItem;
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