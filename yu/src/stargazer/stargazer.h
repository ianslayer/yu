
namespace yu
{
struct WorkItem;

extern struct StarGazer* gStarGazer;

void Clear(StarGazer* starGazer);
void SubmitWork(StarGazer* starGazer);

void InitStarGazer(Window& win);
void FreeStarGazer();

}