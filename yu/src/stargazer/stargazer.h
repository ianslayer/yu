
namespace yu
{
struct WorkItem;

struct FrameWorkItemResult
{
	WorkItem* frameStartItem;
	WorkItem* frameEndItem;
};

struct StarGazer
{
//pre-defined work
	WorkItem* startWork;
	WorkItem* inputWork;
	WorkItem* cameraControllerWork;
	WorkItem* testRenderer;
	WorkItem* endWork;
};
extern StarGazer* gStarGazer;

void Clear(StarGazer* workMap);
void SubmitWork(StarGazer* workMap);

void InitStarGazer();
void FreeStarGazer();

}