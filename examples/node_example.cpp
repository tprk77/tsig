// Copyright (c) 2021 Tim Perkins

#include <fmt/format.h>

#include <tsig/signal.hpp>
#include <tsig/tn/node.hpp>

struct Point {
  int x;
  int y;
};

struct PointCloud {
  std::vector<Point> points;
};

struct VehiclePose {
  Point point;
};

struct ObstacleDetections {
  std::vector<Point> points;
};

struct DetectorDiag {
  int num_detections;
};

using ObstacleDetectorNode =  //
    tsig::tn::Node<tsig::tn::WithInputs<PointCloud, PointCloud, VehiclePose>,
                   tsig::tn::WithOutputs<ObstacleDetections, DetectorDiag>>;

using ObstacleAvoidanceNode =  //
    tsig::tn::Node<tsig::tn::WithInputs<ObstacleDetections>, tsig::tn::WithoutOutputs>;

class ObstacleDetectorTask {
 public:
  // These constants are optional but help readability
  static constexpr size_t INPUT_PONT_CLOUD1 = 0;
  static constexpr size_t INPUT_PONT_CLOUD2 = 1;
  static constexpr size_t INPUT_VEHICLE_POSE = 2;
  static constexpr size_t OUTPUT_OBSTACLE_DETECTIONS = 0;
  static constexpr size_t OUTPUT_DETECTOR_DIAG = 1;

  ObstacleDetectorTask() = default;

  tsig::tn::DataHandlerTuple<PointCloud, PointCloud, VehiclePose> GetHandlers()
  {
    using namespace std::placeholders;
    return {std::bind(&ObstacleDetectorTask::HandlePointCloud1, this, _1),
            std::bind(&ObstacleDetectorTask::HandlePointCloud2, this, _1),
            std::bind(&ObstacleDetectorTask::HandleVehiclePose, this, _1)};
  }

  void SetSinks(tsig::tn::DataSinkTuple<ObstacleDetections, DetectorDiag> sinks)
  {
    // Ideally... the sinks would be const so it's enforced that they never
    // change after construction. Make a more advanced builder?
    output_detection_sink_ = std::get<0>(sinks);
    detector_diag_sink_ = std::get<1>(sinks);
  }

  void HandlePointCloud1(const PointCloud& pc)
  {
    pc1_ = pc;
    got_pc1_ = true;
    if (got_pc2_) {
      merged_pc_ = MergePointClouds_(pc1_, pc2_);
      got_pc1_ = got_pc2_ = false;
      // For now, let's just do Tick right here
      Tick();
    }
  }

  void HandlePointCloud2(const PointCloud& pc)
  {
    pc2_ = pc;
    got_pc2_ = true;
    if (got_pc1_) {
      merged_pc_ = MergePointClouds_(pc1_, pc2_);
      got_pc1_ = got_pc2_ = false;
      // For now, let's just do Tick right here
      Tick();
    }
  }

  void HandleVehiclePose(const VehiclePose& vp)
  {
    vp_ = vp;
  }

  // TODO How will this work? Process one iteration.
  void Tick()
  {
    ObstacleDetections od = GetDetections_(merged_pc_, vp_);
    output_detection_sink_(od);
    // And publish the diagnostic info as well
    detector_diag_sink_(DetectorDiag{static_cast<int>(od.points.size())});
  }

 private:
  static PointCloud MergePointClouds_(const PointCloud& pc1, const PointCloud& pc2)
  {
    PointCloud merged_pc = {};
    for (const auto& point : pc1.points) {
      merged_pc.points.push_back(point);
    }
    for (const auto& point : pc2.points) {
      merged_pc.points.push_back(point);
    }
    return merged_pc;
  }

  static ObstacleDetections GetDetections_(const PointCloud& pc, const VehiclePose& vp)
  {
    ObstacleDetections od = {};
    for (const auto& point : pc.points) {
      const double dist =
          std::sqrt(static_cast<double>((point.x - vp.point.x) * (point.x - vp.point.x)
                                        + (point.y - vp.point.y) * (point.y - vp.point.y)));
      if (dist < 10.0) {
        od.points.push_back(point);
      }
    }
    return od;
  }

  tsig::tn::DataHandler<ObstacleDetections> output_detection_sink_;
  tsig::tn::DataHandler<DetectorDiag> detector_diag_sink_;

  PointCloud pc1_;
  bool got_pc1_ = false;
  PointCloud pc2_;
  bool got_pc2_ = false;
  PointCloud merged_pc_;
  VehiclePose vp_;
};

class ObstacleAvoidanceTask {
 public:
  // These constants are optional but help readability
  static constexpr size_t INPUT_OBSTACLE_DETECTIONS = 0;

  ObstacleAvoidanceTask() = default;

  tsig::tn::DataHandlerTuple<ObstacleDetections> GetHandlers()
  {
    using namespace std::placeholders;
    return {std::bind(&ObstacleAvoidanceTask::HandleObstacleDetections, this, _1)};
  }

  void SetSinks(tsig::tn::DataSinkTuple<>)
  {
    // Would be nice to be able to remove this completely
  }

  void HandleObstacleDetections(const ObstacleDetections& od)
  {
    for (const auto& point : od.points) {
      fmt::print("Got OD point: x = {}, y = {}\n", point.x, point.y);
    }
  }
};

int main(void)
{
  // The task is ignorant and independent of the node. The task could have complex
  // construction, but here it's just the default constructor.
  ObstacleDetectorTask od_task;

  // Build the node using the constructed task
  ObstacleDetectorNode od_node =
      tsig::tn::NodeBuilder<ObstacleDetectorNode>::Build(std::move(od_task));

  // Same thing but for the second one
  ObstacleAvoidanceTask oa_task;
  ObstacleAvoidanceNode oa_node =
      tsig::tn::NodeBuilder<ObstacleAvoidanceNode>::Build(std::move(oa_task));

  tsig::Signal<void(const PointCloud&)> sig_pc1;
  tsig::Signal<void(const PointCloud&)> sig_pc2;
  tsig::Signal<void(const VehiclePose&)> sig_vp;

  // Connect these up manually for testing
  od_node.Accept<ObstacleDetectorTask::INPUT_PONT_CLOUD1>(sig_pc1);
  od_node.Accept<ObstacleDetectorTask::INPUT_PONT_CLOUD2>(sig_pc2);
  od_node.Accept<ObstacleDetectorTask::INPUT_VEHICLE_POSE>(sig_vp);

  // Connect them up output 0 to input 0
  od_node.Connect<ObstacleDetectorTask::OUTPUT_OBSTACLE_DETECTIONS,
                  ObstacleAvoidanceTask::INPUT_OBSTACLE_DETECTIONS>(oa_node);

  // Another manual connection
  tsig::Sigcon sc =
      od_node.Connect<ObstacleDetectorTask::OUTPUT_DETECTOR_DIAG>([](const DetectorDiag& diag) {
        fmt::print("Got diag: num_detections = {}\n", diag.num_detections);
      });

  // Do some manual sends
  sig_vp.Emit(VehiclePose{Point{0, 0}});
  sig_pc1.Emit(PointCloud{{Point{1, 1}, Point{1, 2}, Point{2, 2}}});
  sig_pc2.Emit(PointCloud{{Point{5, 5}, Point{10, 10}, Point{12, 10}}});

  return 0;
}
