// benchmarks.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define autoc auto const
#define autoce auto constexpr

#include <iostream>
#include <fstream>
#include <chrono>
#include <execution>
#include <span>
#include <functional>
#include <numbers>
#include <vector>
#include <random>

#include "../octree.h"
#include "OrthoTreePointDynamicGeneral.h"


using namespace std::chrono;
using namespace std;

using namespace NTree;


namespace
{
  autoce N = 3;

  autoce szSeparator = "; ";
  autoce szNewLine = "\n";

  autoce rMax = 8.0;

#ifdef _DEBUG
  autoce N1M = 100000;
  autoce NR = 1;
#else
  autoce N1M = 1000000;
  autoce NR = 100;
#endif // _DEBUG

  using time_unit = milliseconds;

  using Adaptor = AdaptorGeneral<N, PointND<N>, BoundingBoxND<N>>;
  autoce degree_to_rad(double degree) { return degree / 180 * std::numbers::pi; }


  template<size_t nDim>
  static constexpr PointND<nDim> CreateBoxMax(PointND<nDim> const& pt, double size)
  {
    auto ptMax = pt;
    for (size_t iDim = 0; iDim < nDim; ++iDim)
      Adaptor::point_comp(ptMax, static_cast<dim_type>(iDim)) += size;

    return ptMax;
  }


  template<dim_type nDim>
  static BoundingBoxND<nDim> CreateSearcBox(double rBegin, double rSize)
  {
    auto box = BoundingBoxND<nDim>{};
    for (dim_type iDim = 0; iDim < nDim; ++iDim)
      box.Min[iDim] = rBegin;

    box.Max = CreateBoxMax<nDim>(box.Min, rSize);
    return box;
  }


  template<dim_type nDim, size_t nNumber>
  static vector<PointND<nDim>> CreatePoints_Diagonal()
  {
    auto aPoint = vector<PointND<nDim>>(nNumber);
    if (nNumber <= 1)
      return aPoint;

    size_t iNumber = 1;

    auto ptMax = PointND<nDim>{};
    for (dim_type iDim = 0; iDim < nDim; ++iDim)
      ptMax[iDim] = rMax;

    // Corner points
    {
      aPoint[1] = ptMax;
      ++iNumber;

      for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim, ++iNumber)
        aPoint[iNumber][iDim] = rMax;
    }

    autoc nNumberPre = iNumber;
    // Angle points
    {
      autoc nRemain = nNumber - iNumber;
      autoc rStep = rMax / static_cast<double>(nRemain + 2);
      for (size_t iRemain = 1; iNumber < nNumber; ++iNumber, ++iRemain)
        for (dim_type iDim = 0; iDim < nDim; ++iDim)
          aPoint[iNumber][iDim] = rMax - iRemain * rStep;

    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(next(begin(aPoint), nNumberPre), end(aPoint), g);

    autoc box = Adaptor::box_of_points(aPoint);
    assert(Adaptor::are_points_equal(box.Max, ptMax, 0.0001));
    assert(Adaptor::are_points_equal(box.Min, PointND<nDim>{}, 0.0001));

    return aPoint;
  }

  template<dim_type nDim, size_t nNumber>
  static vector<PointND<nDim>> CreatePoints_Random()
  {
    auto aPoint = vector<PointND<nDim>>(nNumber);
    if (nNumber <= 1)
      return aPoint;

    size_t iNumber = 1;

    auto ptMax = PointND<nDim>{};
    for (dim_type iDim = 0; iDim < nDim; ++iDim)
      ptMax[iDim] = rMax;


    // Corner points
    {
      aPoint[1] = ptMax;
      ++iNumber;

      for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim, ++iNumber)
        aPoint[iNumber][iDim] = rMax;
    }

    srand(0);
    {
      for (; iNumber < nNumber; ++iNumber)
        for (dim_type iDim = 0; iDim < nDim; ++iDim)
          aPoint[iNumber][iDim] = (rand() % 100) * (rMax / 100.0);

    }

    autoc box = Adaptor::box_of_points(aPoint);
    assert(Adaptor::are_points_equal(box.Max, ptMax, 0.0001));
    assert(Adaptor::are_points_equal(box.Min, PointND<nDim>{}, 0.0001));

    return aPoint;
  }

  template<dim_type nDim, size_t nNumber>
  static vector<PointND<nDim>> CreatePoints_CylindricalSemiRandom()
  {
    auto aPoint = vector<PointND<nDim>>(nNumber);
    if (nNumber <= 1)
      return aPoint;

    size_t iNumber = 1;

    auto ptMax = PointND<nDim>{};
    for (dim_type iDim = 0; iDim < nDim; ++iDim)
      ptMax[iDim] = rMax;

    // Corner points
    {
      aPoint[1] = ptMax;
      ++iNumber;

      for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim, ++iNumber)
        aPoint[iNumber][iDim] = rMax;
    }

    srand(0);
    {
      for (; iNumber < nNumber; ++iNumber)
      {
        autoc rAngle = degree_to_rad(static_cast<double>(rand() % 360));
        autoc rRadius = rMax / 4.0 + (rand() % 100) * 0.01;
        aPoint[iNumber][0] = cos(rAngle) * rRadius + rMax / 2.0;
        aPoint[iNumber][1] = sin(rAngle) * rRadius + rMax / 2.0;

        for (dim_type iDim = 2; iDim < nDim; ++iDim)
          aPoint[iNumber][iDim] = (rand() % 100) * rMax / 100.0;
      }
    }

    autoc box = Adaptor::box_of_points(aPoint);
    assert(Adaptor::are_points_equal(box.Max, ptMax, 0.0001));
    assert(Adaptor::are_points_equal(box.Min, PointND<nDim>{}, 0.0001));

    return aPoint;
  }



  template<dim_type nDim, size_t nNumber>
  static vector<BoundingBoxND<nDim>> CreateBoxes_Diagonal()
  {
    if (nNumber == 0)
      return {};

    autoce rMax = 8.0;
    autoce rUnit = 1.0;
    auto aBox = vector<BoundingBoxND<nDim>>(nNumber);
    aBox[0].Max = CreateBoxMax(PointND<nDim>(), rMax);
    if (nNumber == 1)
      return aBox;

    size_t iNumber = 1;

    // Corner points
    {
      for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim, ++iNumber)
      {
        aBox[iNumber].Min[iDim] = rMax - rUnit;
        aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rUnit);
      }
      if (iNumber == nNumber)
        return aBox;


      for (dim_type iDim = 0; iDim < nDim; ++iDim)
        aBox[iNumber].Min[iDim] = rMax - rUnit;

      aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rUnit);

      ++iNumber;
    }

    // Angle points
    {
      autoc nRemain = nNumber - iNumber;
      autoc rStep = (rMax - rUnit) / (nRemain + 2);
      for (size_t iRemain = 1; iNumber < nNumber; ++iNumber, ++iRemain)
      {
        for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim)
          aBox[iNumber].Min[iDim] = rMax - rUnit - iRemain * rStep;

        aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rStep);
      }
    }

    return aBox;
  }

  template<dim_type nDim, size_t nNumber>
  static vector<BoundingBoxND<nDim>> CreateBoxes_Random()
  {
    if (nNumber == 0)
      return {};

    autoce rMax = 8.0;
    autoce rUnit = 1.0;
    auto aBox = vector<BoundingBoxND<nDim>>(nNumber);
    aBox[0].Max = CreateBoxMax(PointND<nDim>(), rMax);
    if (nNumber == 1)
      return aBox;

    size_t iNumber = 1;

    // Corner points
    {
      for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim, ++iNumber)
      {
        aBox[iNumber].Min[iDim] = rMax - rUnit;
        aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rUnit);
      }
      if (iNumber == nNumber)
        return aBox;


      for (dim_type iDim = 0; iDim < nDim; ++iDim)
        aBox[iNumber].Min[iDim] = rMax - rUnit;

      aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rUnit);

      ++iNumber;
    }

    srand(0);

    {
      for (size_t iRemain = 1; iNumber < nNumber; ++iNumber, ++iRemain)
      {
        autoc iNumberBox = nNumber - iNumber - 1;
        for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim)
          aBox[iNumberBox].Min[iDim] = (rand() % 100) * ((rMax - 1.0) / 100.0);

        aBox[iNumberBox].Max = CreateBoxMax(aBox[iNumberBox].Min, (rand() % 100) * (1.0 * rUnit / 100.0));
      }
    }

    return aBox;
  }

  template<dim_type nDim, size_t nNumber>
  static vector<BoundingBoxND<nDim>> CreateBoxes_CylindricalSemiRandom()
  {
    if (nNumber == 0)
      return {};

    autoce rUnit = 1.0;
    auto aBox = vector<BoundingBoxND<nDim>>(nNumber);
    aBox[0].Max = CreateBoxMax(PointND<nDim>(), rMax);
    if (nNumber == 1)
      return aBox;

    size_t iNumber = 1;

    // Corner points
    {
      for (dim_type iDim = 0; iDim < nDim && iNumber < nNumber; ++iDim, ++iNumber)
      {
        aBox[iNumber].Min[iDim] = rMax - rUnit;
        aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rUnit);
      }
      if (iNumber == nNumber)
        return aBox;


      for (dim_type iDim = 0; iDim < nDim; ++iDim)
        aBox[iNumber].Min[iDim] = rMax - rUnit;

      aBox[iNumber].Max = CreateBoxMax(aBox[iNumber].Min, rUnit);

      ++iNumber;
    }

    srand(0);

    {
      for (size_t iRemain = 1; iNumber < nNumber; ++iNumber, ++iRemain)
      {
        autoc id = nNumber - iNumber - 1;
        autoc rAngle = degree_to_rad(static_cast<double>(rand() % 360));
        autoc rRadius = rMax / 4.0 + (rand() % 100) * 0.01;
        aBox[id].Min[0] = cos(rAngle) * rRadius + rMax / 2.0 - rUnit;
        aBox[id].Min[1] = sin(rAngle) * rRadius + rMax / 2.0 - rUnit;

        for (dim_type iDim = 2; iDim < nDim; ++iDim)
          aBox[id].Min[iDim] = (rand() % 100) * 0.02;

        autoc rSize = (rand() % 100) * 0.01;
        aBox[id].Max = CreateBoxMax(aBox[id].Min, rSize);

      }
    }

    return aBox;
  }

  template <typename box_type>
  vector<vector<NTree::entity_id_type>> RangeSearchNaive(span<box_type> const& vSearchBox, span<box_type> const& vBox)
  {
    autoc n = vBox.size();
    auto vElementFound = vector<vector<NTree::entity_id_type>>();
    vElementFound.reserve(n);
    for (autoc& boxSearch : vSearchBox)
    {
      auto& vElementByBox = vElementFound.emplace_back();
      vElementByBox.reserve(50);

      for (size_t i = 0; i < n; ++i)
        if (AdaptorGeneral::are_boxes_overlapped(boxSearch, vBox[i], true))
          vElementByBox.emplace_back(i);
    }
    return vElementFound;
  }


  template <typename point_type, typename box_type>
  vector<vector<NTree::entity_id_type>> RangeSearchNaive(span<box_type> const& vSearchBox, span<point_type> const& vPoint)
  {
    autoc n = vPoint.size();
    auto vElementFound = vector<vector<NTree::entity_id_type>>();
    vElementFound.reserve(n);
    for (autoc& boxSearch : vSearchBox)
    {
      auto& vElementByBox = vElementFound.emplace_back();
      vElementByBox.reserve(50);

      for (size_t i = 0; i < n; ++i)
        if (AdaptorGeneral<N, point_type, box_type>::does_box_contain_point(boxSearch, vPoint[i]))
          vElementByBox.emplace_back(i);
    }
    return vElementFound;
  }


  template<size_t nDim>
  static size_t TreePointCreate(size_t depth, std::span<PointND<nDim> const> const& aPoint, bool fPar = false)
  {
    auto box = BoundingBoxND<nDim>{};
    box.Max.fill(rMax);

    auto nt = fPar
      ? TreePointND<nDim>::template Create<std::execution::parallel_unsequenced_policy>(aPoint, depth, box)
      : TreePointND<nDim>::Create(aPoint, depth, box)
      ;

    return nt.GetNodes().size();
  }

  template<size_t nDim>
  static size_t TreePointNaiveCreate(size_t depth, std::span<PointND<nDim> const> const& aPoint, bool)
  {
    auto box = BoundingBoxND<nDim>{};
    box.Max.fill(rMax);

    auto nt = OrthoTreePointDynamicND<nDim>::Create(aPoint, depth, box, 11);
    return nt.GetNodeSize();
  }


  template<size_t nDim>
  static size_t TreeBoxCreate(size_t depth, std::span<BoundingBoxND<nDim> const> const& aBox, bool fPar = false)
  {
    auto box = BoundingBoxND<nDim>{};
    box.Max.fill(rMax);

    auto nt = fPar
      ? TreeBoxND<nDim>::template Create<std::execution::parallel_unsequenced_policy>(aBox, depth, box)
      : TreeBoxND<nDim>::Create(aBox, depth, box)
      ;

    return nt.GetNodes().size();
  }

  template<size_t nDim>
  static size_t TreeBoxDynCreate(size_t depth, std::span<BoundingBoxND<nDim> const> const& aBox, bool)
  {
    auto box = BoundingBoxND<nDim>{};
    box.Max.fill(rMax);

    auto nt = OrthoTreeBoxDynamicND<nDim>::Create(aBox, depth, box, 11);
    return nt.GetNodeSize();
  }
  

  void display_time(microseconds const& time)
  {
    if (time.count() < 10000)
      std::cout << time;
    else
      std::cout << duration_cast<milliseconds>(time);
  }

  inline double microseconds_to_double(microseconds const& time)
  {
    return static_cast<double>(time.count()) / 1000.0;
  }

  template<typename entity_type>
  struct MeasurementTask
  {
    string szDisplay;
    int nDataSize;
    int nRepeat = 10;
    size_t nDepth = 5;
    bool fParallel = false;
    span<entity_type const> sEntity;
    function<size_t(size_t, span<entity_type const>, bool)> func;

    size_t Run() const { return func(nDepth, sEntity, fParallel); }
  };


  template<size_t N, typename product_type>
  product_type GenerateGeometry(std::function<product_type(void)> fn, string const& szName, double M, ofstream& report)
  {
    std::cout << "Generate " << szName << "...";

    autoc t0 = high_resolution_clock::now();
    product_type const product = fn();
    autoc t1 = high_resolution_clock::now();
    autoc tDur = duration_cast<microseconds>(t1 - t0);

    std::cout << " Finished. ";
    display_time(tDur);
    std::cout << "\n";

    report
      << szName << szSeparator
      << 1 << szSeparator
      << string("seq") << szSeparator
      << M * N1M << szSeparator
      << microseconds_to_double(tDur) << szSeparator
      << szNewLine
      ;

    return product;
  }


  autoce aSizeNonLog = array{ 100, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000, 6000, 7000, 8000, 10000, 12000, 15000 };
  autoce nSizeNonLog = aSizeNonLog.size();
  autoce aRepeatNonLog = array{ 1000 * NR, 1000 * NR, 1000 * NR, 100 * NR, 100 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR, 10 * NR };
  static_assert(nSizeNonLog == aRepeatNonLog.size());

  autoce aSizeLog = array{ 10, 50, 100, 1000, 2500, 5000, 7500, 10000, 100000, N1M, 10 * N1M, 100 * N1M };
  autoce nSizeLog = aSizeLog.size();
  autoce aRepeatLog = array{ 100000, 100000, 10000, 2000, 1000, 500, 500, 100, 100, 10, 10, 5 };
  //static_assert(nSizeLog == aRepeatLog.size());


  template<size_t N>
  vector<MeasurementTask<PointND<N>>> GeneratePointTasks(size_t nDepth, string const& szName, span<PointND<N> const> const& sPoint)
  {
    auto vTask = vector<MeasurementTask<PointND<N>>>();
    for (autoc fPar : { false, true })
      for (size_t iSize = 0; iSize < nSizeLog; ++iSize)
        vTask.push_back(MeasurementTask<PointND<N>>{ szName, aSizeLog[iSize], aRepeatLog[iSize], nDepth, fPar, sPoint.subspan(0, aSizeLog[iSize]), TreePointCreate<N> });

    return vTask;
  }

  template<size_t N>
  vector<MeasurementTask<PointND<N>>> GeneratePointTasks_NonLog(size_t nDepth, string const& szName, span<PointND<N> const> const& sPoint)
  {
    auto vTask = vector<MeasurementTask<PointND<N>>>();
    for (size_t iSize = 0; iSize < nSizeNonLog; ++iSize)
      vTask.push_back(MeasurementTask<PointND<N>>{ szName, aSizeNonLog[iSize], aRepeatNonLog[iSize], nDepth, false, sPoint.subspan(0, aSizeNonLog[iSize]), TreePointCreate<N> });

    return vTask;
  }


  template<size_t N>
  vector<MeasurementTask<PointND<N>>> GeneratePointDynTasks_NonLog(size_t nDepth, string const& szName, span<PointND<N> const> const& sPoint)
  {
    auto vTask = vector<MeasurementTask<PointND<N>>>();
    for (size_t iSize = 0; iSize < nSizeLog; ++iSize)
      vTask.push_back(MeasurementTask<PointND<N>>{ szName, aSizeLog[iSize], aRepeatLog[iSize], nDepth, false, sPoint.subspan(0, aSizeLog[iSize]), TreePointNaiveCreate<N> });

    return vTask;
  }

  template<size_t N>
  vector<MeasurementTask<BoundingBoxND<N>>> GenerateBoxDynTasks_NonLog(size_t nDepth, string const& szName, span<BoundingBoxND<N> const> const& sBox)
  {
    auto vTask = vector<MeasurementTask<BoundingBoxND<N>>>();
    for (size_t iSize = 0; iSize < nSizeLog; ++iSize)
      vTask.push_back(MeasurementTask<BoundingBoxND<N>>{ szName, aSizeLog[iSize], aRepeatLog[iSize], nDepth, false, sBox.subspan(0, aSizeLog[iSize]), TreeBoxDynCreate<N> });

    return vTask;
  }


  template<size_t N>
  vector<MeasurementTask<PointND<N>>> SearchTreePointTasks(size_t nDepth, string const& szName, span<PointND<N> const> const& sPoint, span<BoundingBoxND<N> const> const& vSearchBox, double rPercentage)
  {
    auto vTask = vector<MeasurementTask<PointND<N>>>();
    for (autoc fPar : { false, true })
      for (size_t iSize = 0; iSize < nSizeNonLog; ++iSize)
        vTask.push_back(MeasurementTask<PointND<N>>{ szName, aSizeNonLog[iSize], aRepeatNonLog[iSize], nDepth, fPar, sPoint.subspan(0, aSizeNonLog[iSize]), [&vSearchBox, rPercentage](size_t nDepth, span<PointND<N> const> const& aPoint, bool fPar)
        {
          autoc n = aPoint.size();
          autoc aSearchBox = vSearchBox.subspan(0, static_cast<size_t>(n * rPercentage / 100.0));

          autoc nt = fPar
            ? TreePointND<N>::template Create<std::execution::parallel_unsequenced_policy>(aPoint, nDepth)
            : TreePointND<N>::Create(aPoint, nDepth)
            ;

          size_t nFound = 0;
          for (autoc& boxSearch : aSearchBox)
          {
            autoc vElem = nt.RangeSearch(boxSearch, aPoint, true);
            nFound += vElem.size();
          }

          return nFound;
        } });

    return vTask;
  }

  template<size_t N>
  vector<MeasurementTask<PointND<N>>> SearchBruteForcePointTasks(size_t nDepth, string const& szName, span<PointND<N> const> const& sPoint, span<BoundingBoxND<N> const> const& vSearchBox, double rPercentage)
  {
    auto vTask = vector<MeasurementTask<PointND<N>>>();
    for (size_t iSize = 0; iSize < nSizeNonLog; ++iSize)
      vTask.push_back(MeasurementTask<PointND<N>>
        { szName, aSizeNonLog[iSize], aRepeatNonLog[iSize], nDepth, false, sPoint.subspan(0, aSizeNonLog[iSize])
        , [&vSearchBox, rPercentage](size_t nDepth, span<PointND<N> const> const& aPoint, bool fPar)
          {
            autoc n = aPoint.size();
            autoc aSearchBox = vSearchBox.subspan(0, static_cast<size_t>(n * rPercentage / 100.0));
            autoc vvElem = RangeSearchNaive(aSearchBox, aPoint);

            size_t nFound = 0;
            for (autoc& vElem : vvElem)
              nFound += vElem.size();

            return nFound;
          }
        }
      );

    return vTask;
  }



  template<size_t N>
  vector<MeasurementTask<BoundingBoxND<N>>> GenerateBoxTasks(size_t nDepth, string szName, span<BoundingBoxND<N> const> const& sBox)
  {
    auto vTask = vector<MeasurementTask<BoundingBoxND<N>>>();
    for (autoc fPar : { false, true })
      for (int iSize = 0; iSize < nSizeLog; ++iSize)
        vTask.push_back(MeasurementTask<BoundingBoxND<N>>{ szName, aSizeLog[iSize], aRepeatLog[iSize], nDepth, fPar, sBox.subspan(0, aSizeLog[iSize]), TreeBoxCreate<N> });

    return vTask;
  }


  tuple<microseconds, size_t> Measure(int nRepeat, function<size_t(void)> const& fn)
  {
    autoc t0 = high_resolution_clock::now();
    auto v = vector<size_t>(nRepeat);
    for (int i = 0; i < nRepeat; ++i)
      v[i] = fn();

    autoc t1 = high_resolution_clock::now();
    return { duration_cast<microseconds>((t1 - t0) / nRepeat), v[0] };
  }


  template<typename entity_type>
  void RunTasks(vector<MeasurementTask<entity_type>> const& vTask, ofstream& report)
  {
    for (autoc& task : vTask)
    {
      autoc szPar = (task.fParallel ? string("par") : string("unseq"));
      std::cout << "Create tree for: " << task.szDisplay << " " << task.nDataSize << " " << szPar << " Repeat: " << task.nRepeat << "...";
      autoc[tDur, nNode] = Measure(task.nRepeat, [&task]{ return task.Run(); });

      std::cout << " Finished. ";
      display_time(tDur);
      std::cout << szNewLine;

      report
        << task.szDisplay << szSeparator
        << task.nRepeat << szSeparator
        << szPar << szSeparator
        << task.nDataSize << szSeparator
        << microseconds_to_double(tDur) << szSeparator
        << nNode << szSeparator
        << szNewLine
        ;
    }
  }
}


int main()
{

  ofstream report;
  report.open("report.csv");

  autoce nDepth = 5;
  
  {
    autoc szName = string("Diagonally placed points");
    autoc aPointDiag_100M = GenerateGeometry<N, vector<PointND<N>>>([&] { return CreatePoints_Diagonal<N, 100 * N1M>(); }, szName, 100, report);
    autoc vTask = GeneratePointTasks<N>(nDepth, szName, aPointDiag_100M);
    RunTasks(vTask, report);
  }

  {
    autoc szName = string("Random placed points");
    autoc aPointDiag_100M = GenerateGeometry<N, vector<PointND<N>>>([&] { return CreatePoints_Random<N, 100 * N1M>(); }, szName, 100, report);
    autoc vTask = GeneratePointTasks<N>(nDepth, szName, aPointDiag_100M);
    RunTasks(vTask, report);
  }

  {
    autoc szName = string("Cylindrical semi-random placed points");
    autoc aPointDiag_100M = GenerateGeometry<N, vector<PointND<N>>>([&] { return CreatePoints_CylindricalSemiRandom<N, 100 * N1M>(); }, szName, 100, report);
    autoc vTask = GeneratePointTasks<N>(nDepth, szName, aPointDiag_100M);
    RunTasks(vTask, report);
  }

  {
    autoc szName = string("Diagonally placed boxes");
    autoc aPointDiag_100M = GenerateGeometry<N, vector<BoundingBoxND<N>>>([&] { return CreateBoxes_Diagonal<N, 100 * N1M>(); }, szName, 100, report);
    autoc vTask = GenerateBoxTasks<N>(nDepth, szName, aPointDiag_100M);
    RunTasks(vTask, report);
  }

  {
    autoc szName = string("Random placed boxes");
    autoc aPointDiag_100M = GenerateGeometry<N, vector<BoundingBoxND<N>>>([&] { return CreateBoxes_Random<N, 100 * N1M>(); }, szName, 100, report);
    autoc vTask = GenerateBoxTasks<N>(nDepth, szName, aPointDiag_100M);
    RunTasks(vTask, report);
  }

  {
    autoc szName = string("Cylindrical semi-random placed boxes");
    autoc aPointDiag_100M = GenerateGeometry<N, vector<BoundingBoxND<N>>>([&] { return CreateBoxes_CylindricalSemiRandom<N, 100 * N1M>(); }, szName, 100, report);
    autoc vTask = GenerateBoxTasks<N>(nDepth, szName, aPointDiag_100M);
    RunTasks(vTask, report);
  }
  
  
  // Morton vs Dynamic
  {
    autoc szName = string("Cylindrical semi-random placed points Morton vs Dynamic");
    autoc aPoint = GenerateGeometry<N, vector<PointND<N>>>([&] { return CreatePoints_Random<N, 10*N1M>(); }, szName, 10, report);
    autoc aBox = GenerateGeometry<N, vector<BoundingBoxND<N>>>([&] { return CreateBoxes_Random<N, 10 * N1M>(); }, szName, 10, report);

    autoc vTaskMortonP = GeneratePointTasks<N>(nDepth, "Morton point", aPoint);
    autoc vTaskDynP = GeneratePointDynTasks_NonLog<N>(nDepth, "Dynamic point", aPoint);
    
    autoc vTaskMortonB = GenerateBoxTasks<N>(nDepth, "Morton box", aBox);
    autoc vTaskDynB = GenerateBoxDynTasks_NonLog<N>(nDepth, "Dynamic box", aBox);

    RunTasks(vTaskMortonP, report);
    RunTasks(vTaskDynP, report);
    RunTasks(vTaskMortonB, report);
    RunTasks(vTaskDynB, report);

  }
  
  // Search
  
  // Range search: Brute force vs Octree
  {
    autoc szName = string("Search: Cylindrical semi-random placed point NoPt/NoBox:10%");
    autoc aPoint = GenerateGeometry<N, vector<PointND<N>>>([&] { return CreatePoints_CylindricalSemiRandom<N, N1M>(); }, szName, N1M, report);
    autoce M = 0.1;
    autoc aSearchBox = GenerateGeometry<N, vector<BoundingBoxND<N>>>([&] { return CreateBoxes_CylindricalSemiRandom<N, static_cast<size_t>(N1M * M)>(); }, szName, M, report);
    autoce rPercentage = 10.0;
    autoc vTaskBruteForce = SearchBruteForcePointTasks<N>(nDepth, "Point range-search naive", aPoint, aSearchBox, rPercentage);
    autoc vTaskTree = SearchTreePointTasks<N>(6, "Point range-search by octree", aPoint, aSearchBox, rPercentage);

    RunTasks(vTaskBruteForce, report);
    RunTasks(vTaskTree, report);
  }

  report.close();
}