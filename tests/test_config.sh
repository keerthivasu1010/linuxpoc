#!/bin/sh
set -eu
[ -x bin/adas_supervisor ]
[ -x bin/camera_service ]
[ -x bin/sensor_service ]
[ -x bin/object_detection_service ]
[ -x bin/lane_detection_service ]
grep -q '^camera ' config/services.conf
grep -q '^sensor ' config/services.conf
grep -q '^object_detection ' config/services.conf
grep -q '^lane_detection ' config/services.conf
echo "Basic project validation: PASS"
