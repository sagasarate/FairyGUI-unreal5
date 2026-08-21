#include "Widgets/SMovieClip.h"

FMovieClipData::FMovieClipData() :
    Interval(0),
    RepeatDelay(0),
    bSwing(false)
{
}

void FMovieClipData::AddReferencedObjects(FReferenceCollector& Collector)
{
    for (auto& Frame : Frames)
    {
        if (Frame.Texture != nullptr)
            Collector.AddReferencedObject(Frame.Texture);
    }
}

FString FMovieClipData::GetReferencerName() const
{
    return TEXT("FMovieClipData");
}

IMPLEMENT_TYPE_ID(SMovieClip)

SMovieClip::SMovieClip(UGObject* InGObject)
    : SFImage(InGObject)
    , Frame(0)
    , bPlaying(true)
    , TimeScale(1.0f)
    , Start(0)
    , End(-1)
    , Times(0)
    , EndAt(-1)
    , Status(0)
    , FrameElapsed(0.0f)
    , bReversed(false)
    , RepeatedCount(0)
{
}

void SMovieClip::SetClipData(TSharedPtr<FMovieClipData> InData)
{
    Data = InData;

    SetScale9Grid(TOptional<FBox2D>());
    SetScaleByTile(false);

    if (!Data.IsValid())
    {
        SetTexture(nullptr);
        return;
    }
    int32 frameCount = Data->Frames.Num();

    if (End == -1 || End > frameCount - 1)
        End = frameCount - 1;
    if (EndAt == -1 || EndAt > frameCount - 1)
        EndAt = frameCount - 1;

    if (Frame < 0 || Frame > frameCount - 1)
        Frame = frameCount - 1;

    FrameElapsed = 0;
    RepeatedCount = 0;
    bReversed = false;

    DrawFrame();
}

void SMovieClip::SetPlaying(bool InPlaying)
{
    bPlaying = InPlaying;
}

void SMovieClip::SetTimeScale(float InTimeScale)
{
    TimeScale = InTimeScale;
}

void SMovieClip::SetFrame(int32 InFrame)
{
    if (!Data.IsValid())
    {
        Frame = InFrame;
        return;
    }

    int32 frameCount = Data->Frames.Num();

    if (InFrame >= frameCount)
        InFrame = frameCount - 1;

    Frame = InFrame;
    FrameElapsed = 0;
    DrawFrame();
}

void SMovieClip::Advance(float Time)
{
    if (!Data.IsValid())
        return;

    int32 frameCount = Data->Frames.Num();
    if (frameCount == 0)
        return;

    int32 beginFrame = Frame;
    bool beginReversed = bReversed;
    float backupTime = Time;
    while (true)
    {
        float tt = Data->Interval + Data->Frames[Frame].AddDelay;
        if (Frame == 0 && RepeatedCount > 0)
            tt += Data->RepeatDelay;
        if (Time < tt)
        {
            FrameElapsed = 0;
            break;
        }

        Time -= tt;

        if (Data->bSwing)
        {
            if (bReversed)
            {
                Frame--;
                if (Frame <= 0)
                {
                    Frame = 0;
                    RepeatedCount++;
                    bReversed = !bReversed;
                }
            }
            else
            {
                Frame++;
                if (Frame > frameCount - 1)
                {
                    Frame = FMath::Max(0, frameCount - 2);
                    RepeatedCount++;
                    bReversed = !bReversed;
                }
            }
        }
        else
        {
            Frame++;
            if (Frame > frameCount - 1)
            {
                Frame = 0;
                RepeatedCount++;
            }
        }

        if (Frame == beginFrame && bReversed == beginReversed)
        {
            float roundTime = backupTime - Time;
            Time -= FMath::FloorToInt(Time / roundTime) * roundTime;
        }
    }
}

void SMovieClip::SetPlaySettings(int32 InStart, int32 InEnd, int32 InTimes, int32 InEndAt, const FSimpleDelegate& InCompleteCallback)
{
    int32 frameCount = Data.IsValid() ? Data->Frames.Num() : 0;

    Start = InStart;
    End = InEnd;
    if (End == -1 || End > frameCount - 1)
        End = frameCount - 1;
    Times = InTimes;
    EndAt = InEndAt;
    if (EndAt == -1)
        EndAt = End;
    Status = 0;
    CompleteCallback = InCompleteCallback;

    SetFrame(InStart);
}

void SMovieClip::DrawFrame()
{
    if (Data.IsValid() && Frame < Data->Frames.Num())
        SetTexture(Data->Frames[Frame].Texture);
}

void SMovieClip::CollectRenderUnits(FRenderGroup* ParentGroup, const FGeometry& AllottedGeometry, float InAlpha,
    float InSaturation) const
{
    // 在自定义渲染管线中执行动画 Tick（绕过标准 Slate Paint→Tick 链路）
    SMovieClip* MutableThis = const_cast<SMovieClip*>(this);

    if (Data.IsValid() && bPlaying)
    {
        int32 frameCount = Data->Frames.Num();
        if (frameCount > 0 && Status != 3)
        {
            float dt = FSlateApplication::Get().GetDeltaTime();
            if (TimeScale != 1)
                dt *= TimeScale;

            MutableThis->FrameElapsed += dt;
            float tt = Data->Interval + Data->Frames[Frame].AddDelay;
            if (Frame == 0 && RepeatedCount > 0)
                tt += Data->RepeatDelay;
            if (FrameElapsed < tt)
                goto DoneTick;

            MutableThis->FrameElapsed -= tt;
            if (FrameElapsed > Data->Interval)
                MutableThis->FrameElapsed = Data->Interval;

            if (Data->bSwing)
            {
                if (bReversed)
                {
                    MutableThis->Frame--;
                    if (Frame <= 0)
                    {
                        MutableThis->Frame = 0;
                        MutableThis->RepeatedCount++;
                        MutableThis->bReversed = !bReversed;
                    }
                }
                else
                {
                    MutableThis->Frame++;
                    if (Frame > frameCount - 1)
                    {
                        MutableThis->Frame = FMath::Max(0, frameCount - 2);
                        MutableThis->RepeatedCount++;
                        MutableThis->bReversed = !bReversed;
                    }
                }
            }
            else
            {
                MutableThis->Frame++;
                if (Frame > frameCount - 1)
                {
                    MutableThis->Frame = 0;
                    MutableThis->RepeatedCount++;
                }
            }

            if (Status == 1) //new loop
            {
                MutableThis->Frame = Start;
                MutableThis->FrameElapsed = 0;
                MutableThis->Status = 0;
            }
            else if (Status == 2) //ending
            {
                MutableThis->Frame = EndAt;
                MutableThis->FrameElapsed = 0;
                MutableThis->Status = 3; //ended

                CompleteCallback.ExecuteIfBound();
            }
            else
            {
                if (Frame == End)
                {
                    if (Times > 0)
                    {
                        MutableThis->Times--;
                        if (Times == 0)
                            MutableThis->Status = 2; //ending
                        else
                            MutableThis->Status = 1; //new loop
                    }
                    else if (Start != 0)
                        MutableThis->Status = 1; //new loop
                }
            }

            MutableThis->DrawFrame();
        }
    }
DoneTick:

    SFImage::CollectRenderUnits(ParentGroup, AllottedGeometry, InAlpha, InSaturation);
}
