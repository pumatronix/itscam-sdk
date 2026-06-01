// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// AUTO-GENERATED FILE -- DO NOT EDIT.
// Regenerate with `make codegen` (see tools/codegen/README.md).
//
// Generated from an OpenAPI 3.0 snapshot of the ITSCAM camera webapp.
// Edit tools/codegen/codegen.mjs and rerun, do not patch this output.
// Code generated from JSON Schema using quicktype. DO NOT EDIT.
// To parse and unparse this JSON data, add this code to your project and do:
//
//    profileConfig, err := UnmarshalProfileConfig(bytes)
//    bytes, err = profileConfig.Marshal()
//
//    ocrConfig, err := UnmarshalOcrConfig(bytes)
//    bytes, err = ocrConfig.Marshal()
//
//    analyticsConfig, err := UnmarshalAnalyticsConfig(bytes)
//    bytes, err = analyticsConfig.Marshal()
//
//    classifierConfig, err := UnmarshalClassifierConfig(bytes)
//    bytes, err = classifierConfig.Marshal()
//
//    autoFocus, err := UnmarshalAutoFocus(bytes)
//    bytes, err = autoFocus.Marshal()
//
//    streamConfig, err := UnmarshalStreamConfig(bytes)
//    bytes, err = streamConfig.Marshal()
//
//    misc, err := UnmarshalMisc(bytes)
//    bytes, err = misc.Marshal()
//
//    miscVolatile, err := UnmarshalMiscVolatile(bytes)
//    bytes, err = miscVolatile.Marshal()
//
//    itscamproConfig, err := UnmarshalItscamproConfig(bytes)
//    bytes, err = itscamproConfig.Marshal()
//
//    itscamproStatus, err := UnmarshalItscamproStatus(bytes)
//    bytes, err = itscamproStatus.Marshal()
//
//    imageSignConfig, err := UnmarshalImageSignConfig(bytes)
//    bytes, err = imageSignConfig.Marshal()
//
//    fTPConfig, err := UnmarshalFTPConfig(bytes)
//    bytes, err = fTPConfig.Marshal()
//
//    linceConfig, err := UnmarshalLinceConfig(bytes)
//    bytes, err = linceConfig.Marshal()
//
//    linceStatus, err := UnmarshalLinceStatus(bytes)
//    bytes, err = linceStatus.Marshal()
//
//    vehicleIndicatorConfig, err := UnmarshalVehicleIndicatorConfig(bytes)
//    bytes, err = vehicleIndicatorConfig.Marshal()
//
//    protocolsConfig, err := UnmarshalProtocolsConfig(bytes)
//    bytes, err = protocolsConfig.Marshal()
//
//    profileTransitioner, err := UnmarshalProfileTransitioner(bytes)
//    bytes, err = profileTransitioner.Marshal()
//
//    lanesConfig, err := UnmarshalLanesConfig(bytes)
//    bytes, err = lanesConfig.Marshal()
//
//    ioConfig, err := UnmarshalIoConfig(bytes)
//    bytes, err = ioConfig.Marshal()
//
//    ioBasic, err := UnmarshalIoBasic(bytes)
//    bytes, err = ioBasic.Marshal()
//
//    rESTAPIClientConfig, err := UnmarshalRESTAPIClientConfig(bytes)
//    bytes, err = rESTAPIClientConfig.Marshal()
//
//    rESTAPIClientStatus, err := UnmarshalRESTAPIClientStatus(bytes)
//    bytes, err = rESTAPIClientStatus.Marshal()
//
//    licenses, err := UnmarshalLicenses(bytes)
//    bytes, err = licenses.Marshal()

package itscam

import "encoding/json"

func UnmarshalProfileConfig(data []byte) (ProfileConfig, error) {
	var r ProfileConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ProfileConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalOcrConfig(data []byte) (OcrConfig, error) {
	var r OcrConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *OcrConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalAnalyticsConfig(data []byte) (AnalyticsConfig, error) {
	var r AnalyticsConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *AnalyticsConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalClassifierConfig(data []byte) (ClassifierConfig, error) {
	var r ClassifierConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ClassifierConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalAutoFocus(data []byte) (AutoFocus, error) {
	var r AutoFocus
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *AutoFocus) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalStreamConfig(data []byte) (StreamConfig, error) {
	var r StreamConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *StreamConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalMisc(data []byte) (Misc, error) {
	var r Misc
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *Misc) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalMiscVolatile(data []byte) (MiscVolatile, error) {
	var r MiscVolatile
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *MiscVolatile) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalItscamproConfig(data []byte) (ItscamproConfig, error) {
	var r ItscamproConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ItscamproConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalItscamproStatus(data []byte) (ItscamproStatus, error) {
	var r ItscamproStatus
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ItscamproStatus) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalImageSignConfig(data []byte) (ImageSignConfig, error) {
	var r ImageSignConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ImageSignConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalFTPConfig(data []byte) (FTPConfig, error) {
	var r FTPConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *FTPConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalLinceConfig(data []byte) (LinceConfig, error) {
	var r LinceConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *LinceConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalLinceStatus(data []byte) (LinceStatus, error) {
	var r LinceStatus
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *LinceStatus) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalVehicleIndicatorConfig(data []byte) (VehicleIndicatorConfig, error) {
	var r VehicleIndicatorConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *VehicleIndicatorConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalProtocolsConfig(data []byte) (ProtocolsConfig, error) {
	var r ProtocolsConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ProtocolsConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalProfileTransitioner(data []byte) (ProfileTransitioner, error) {
	var r ProfileTransitioner
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *ProfileTransitioner) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalLanesConfig(data []byte) (LanesConfig, error) {
	var r LanesConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *LanesConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalIoConfig(data []byte) (IoConfig, error) {
	var r IoConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *IoConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalIoBasic(data []byte) (IoBasic, error) {
	var r IoBasic
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *IoBasic) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalRESTAPIClientConfig(data []byte) (RESTAPIClientConfig, error) {
	var r RESTAPIClientConfig
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *RESTAPIClientConfig) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalRESTAPIClientStatus(data []byte) (RESTAPIClientStatus, error) {
	var r RESTAPIClientStatus
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *RESTAPIClientStatus) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

func UnmarshalLicenses(data []byte) (Licenses, error) {
	var r Licenses
	err := json.Unmarshal(data, &r)
	return r, err
}

func (r *Licenses) Marshal() ([]byte, error) {
	return json.Marshal(r)
}

// Camera profile configuration
type ProfileConfig struct {
	Active            *bool                      `json:"active,omitempty"`
	Advanced          *Advanced                  `json:"advanced,omitempty"`
	Color             *Color                     `json:"color,omitempty"`
	Description       *string                    `json:"description,omitempty"`
	Exposure          *Exposure                  `json:"exposure,omitempty"`
	Hdr               *Hdr                       `json:"hdr,omitempty"`
	ID                int64                      `json:"id"`
	Lens              *ProfileConfigLens         `json:"lens,omitempty"`
	MOVFilter         *MOVFilter                 `json:"movFilter,omitempty"`
	MultipleExposures *MultipleExposures         `json:"multipleExposures,omitempty"`
	Name              *string                    `json:"name,omitempty"`
	Overlay           *Overlay                   `json:"overlay,omitempty"`
	Transitions       *Transitions               `json:"transitions,omitempty"`
	Trigger           *Trigger                   `json:"trigger,omitempty"`
	Whitebalance      *ProfileConfigWhitebalance `json:"whitebalance,omitempty"`
}

type Advanced struct {
	Exposition   *Exposition           `json:"exposition,omitempty"`
	Iris         *AdvancedIris         `json:"iris,omitempty"`
	Whitebalance *AdvancedWhitebalance `json:"whitebalance,omitempty"`
}

type Exposition struct {
	PreferredShutter *int64   `json:"preferredShutter,omitempty"`
	UpdateFactor     *float64 `json:"updateFactor,omitempty"`
	UpdateRate       *int64   `json:"updateRate,omitempty"`
}

type AdvancedIris struct {
	UpdateRate *int64 `json:"updateRate,omitempty"`
}

type AdvancedWhitebalance struct {
	UpdateRate *int64 `json:"updateRate,omitempty"`
}

// Camera color configuration fields
type Color struct {
	Blacklevel *int64             `json:"blacklevel,omitempty"`
	Brightness *int64             `json:"brightness,omitempty"`
	Contrast   *int64             `json:"contrast,omitempty"`
	Gain       *WhitebalanceClass `json:"gain,omitempty"`
	Gamma      *int64             `json:"gamma,omitempty"`
	Saturation *int64             `json:"saturation,omitempty"`
}

// Single RGB value in float format
type WhitebalanceClass struct {
	Blue  *float64 `json:"blue,omitempty"`
	Green *float64 `json:"green,omitempty"`
	Red   *float64 `json:"red,omitempty"`
}

// Camera exposure configuration fields
type Exposure struct {
	Gain    *ShutterClass `json:"gain,omitempty"`
	Iris    *ExposureIris `json:"iris,omitempty"`
	Level   *Level        `json:"level,omitempty"`
	Shutter *ShutterClass `json:"shutter,omitempty"`
}

// Shutter attributes
type ShutterClass struct {
	Automatic  *bool  `json:"automatic,omitempty"`
	FixedValue *int64 `json:"fixedValue,omitempty"`
	MaxValue   *int64 `json:"maxValue,omitempty"`
	MinValue   *int64 `json:"minValue,omitempty"`
}

type ExposureIris struct {
	Automatic  *bool  `json:"automatic,omitempty"`
	FixedValue *int64 `json:"fixedValue,omitempty"`
}

type Level struct {
	HoldTime    *int64     `json:"holdTime,omitempty"`
	Mode        *Mode      `json:"mode,omitempty"`
	Roi         *Roi1Class `json:"roi,omitempty"`
	TargetValue *float64   `json:"targetValue,omitempty"`
	UpdateRate  *int64     `json:"updateRate,omitempty"`
}

type Roi1Class struct {
	Enabled *bool  `json:"enabled,omitempty"`
	X0      *int64 `json:"x0,omitempty"`
	X1      *int64 `json:"x1,omitempty"`
	X2      *int64 `json:"x2,omitempty"`
	X3      *int64 `json:"x3,omitempty"`
	Y0      *int64 `json:"y0,omitempty"`
	Y1      *int64 `json:"y1,omitempty"`
	Y2      *int64 `json:"y2,omitempty"`
	Y3      *int64 `json:"y3,omitempty"`
}

type Hdr struct {
	Enable *bool `json:"enable,omitempty"`
}

// Camera lens configuration fields
type ProfileConfigLens struct {
	Exchanger        *bool  `json:"exchanger,omitempty"`
	Focus            *int64 `json:"focus,omitempty"`
	ZfMirrorProfile0 *bool  `json:"zfMirrorProfile0,omitempty"`
	Zoom             *int64 `json:"zoom,omitempty"`
}

type MOVFilter struct {
	Enabled   *bool      `json:"enabled,omitempty"`
	OnlyCheck *bool      `json:"onlyCheck,omitempty"`
	Roi       *Roi1Class `json:"roi,omitempty"`
	Threshold *float64   `json:"threshold,omitempty"`
}

type MultipleExposures struct {
	Enabled  *bool                     `json:"enabled,omitempty"`
	Settings []MultipleExposuresConfig `json:"settings,omitempty"`
}

// Multiple exposures configuration
type MultipleExposuresConfig struct {
	Flash   *Flash       `json:"flash,omitempty"`
	Gain    *SettingGain `json:"gain,omitempty"`
	Shutter *Shutter     `json:"shutter,omitempty"`
}

// Camera flash configuration
type Flash struct {
	Power []Power `json:"power,omitempty"`
}

type Power struct {
	Out     *int64 `json:"out,omitempty"`
	Percent *int64 `json:"percent,omitempty"`
}

type SettingGain struct {
	PercentageOfCurrent *bool    `json:"percentageOfCurrent,omitempty"`
	Value               *float64 `json:"value,omitempty"`
}

type Shutter struct {
	PercentageOfCurrent *bool    `json:"percentageOfCurrent,omitempty"`
	Value               *float64 `json:"value,omitempty"`
}

type Overlay struct {
	Enable *bool   `json:"enable,omitempty"`
	Text   *string `json:"text,omitempty"`
}

// Camera profile transition configuration
type Transitions struct {
	Lower *Lower `json:"lower,omitempty"`
	Upper *Lower `json:"upper,omitempty"`
}

type Lower struct {
	EndTime   *string  `json:"endTime,omitempty"`
	HoldTime  *int64   `json:"holdTime,omitempty"`
	Level     *float64 `json:"level,omitempty"`
	Profile   *int64   `json:"profile,omitempty"`
	StartTime *string  `json:"startTime,omitempty"`
}

type Trigger struct {
	Enabled         *bool      `json:"enabled,omitempty"`
	Event           *string    `json:"event,omitempty"`
	MinimumInterval *int64     `json:"minimumInterval,omitempty"`
	Port            *int64     `json:"port,omitempty"`
	Roi             *Roi1Class `json:"roi,omitempty"`
	Threshold       *float64   `json:"threshold,omitempty"`
}

// Camera white balance configuration fields
type ProfileConfigWhitebalance struct {
	Automatic *bool              `json:"automatic,omitempty"`
	Weights   *WhitebalanceClass `json:"weights,omitempty"`
}

// Ocr service configuration
type OcrConfig struct {
	Ocr *OcrConfigOcr `json:"ocr,omitempty"`
}

type OcrConfigOcr struct {
	AvgCharHeight       *int64     `json:"avgCharHeight,omitempty"`
	AvgPlateAngle       *float64   `json:"avgPlateAngle,omitempty"`
	AvgPlateSlant       *float64   `json:"avgPlateSlant,omitempty"`
	ClassifierExpansion *float64   `json:"classifierExpansion,omitempty"`
	CountryCode         *int64     `json:"countryCode,omitempty"`
	Enabled             *bool      `json:"enabled,omitempty"`
	Licensed            *bool      `json:"licensed,omitempty"`
	MaxCharHeight       *int64     `json:"maxCharHeight,omitempty"`
	MaxLowProbChars     *int64     `json:"maxLowProbChars,omitempty"`
	MaxPlates           *int64     `json:"maxPlates,omitempty"`
	MinCharHeight       *int64     `json:"minCharHeight,omitempty"`
	MinProbPerChar      *float64   `json:"minProbPerChar,omitempty"`
	ProcessingMode      *int64     `json:"processingMode,omitempty"`
	ProcessingQueue     *int64     `json:"processingQueue,omitempty"`
	ProcessingThreads   *int64     `json:"processingThreads,omitempty"`
	ProcessingTimeout   *int64     `json:"processingTimeout,omitempty"`
	Roi                 *Roi1Class `json:"roi,omitempty"`
	UseClassifierResult *bool      `json:"useClassifierResult,omitempty"`
	VehicleType         *int64     `json:"vehicleType,omitempty"`
}

// Plate Analytics service configuration
type AnalyticsConfig struct {
	Voting *Voting `json:"voting,omitempty"`
}

type Voting struct {
	Enabled                      *bool      `json:"enabled,omitempty"`
	ForwardWithoutPlateIfTracker *bool      `json:"forwardWithoutPlateIfTracker,omitempty"`
	KeepBestOnly                 *bool      `json:"keepBestOnly,omitempty"`
	MaxDiffChars                 *int64     `json:"maxDiffChars,omitempty"`
	Roi1                         *Roi1Class `json:"roi1,omitempty"`
	Roi2                         *Roi1Class `json:"roi2,omitempty"`
	SamePlateDebounce            *int64     `json:"samePlateDebounce,omitempty"`
	UseClassifier                *bool      `json:"useClassifier,omitempty"`
}

// Vehicle Classifier service configuration
type ClassifierConfig struct {
	Classifier *ClassifierConfigClassifier `json:"classifier,omitempty"`
}

type ClassifierConfigClassifier struct {
	EnableCharacteristics   *bool                    `json:"enableCharacteristics,omitempty"`
	Enabled                 *bool                    `json:"enabled,omitempty"`
	EnableSpeed             *bool                    `json:"enableSpeed,omitempty"`
	FirstOnly               *bool                    `json:"firstOnly,omitempty"`
	Licensed                *bool                    `json:"licensed,omitempty"`
	MinProbability          *float64                 `json:"minProbability,omitempty"`
	ModelType               *int64                   `json:"modelType,omitempty"`
	ProcessingQueue         *int64                   `json:"processingQueue,omitempty"`
	ProcessingThreads       *int64                   `json:"processingThreads,omitempty"`
	SceneType               *int64                   `json:"sceneType,omitempty"`
	SpeedCalibrationRegion1 *SpeedCalibrationRegion1 `json:"speedCalibrationRegion1,omitempty"`
	SpeedCalibrationRegion2 *SpeedCalibrationRegion1 `json:"speedCalibrationRegion2,omitempty"`
	TriggerEnabled          *bool                    `json:"triggerEnabled,omitempty"`
	TriggerRegion0          *TriggerRegion0          `json:"triggerRegion0,omitempty"`
	TriggerRegion1          *TriggerRegion0          `json:"triggerRegion1,omitempty"`
	TriggerRegion2          *TriggerRegion0          `json:"triggerRegion2,omitempty"`
	TriggerRegion3          *TriggerRegion0          `json:"triggerRegion3,omitempty"`
}

// Classifier based speed calibration region;
type SpeedCalibrationRegion1 struct {
	P0Top1Sz *float64 `json:"p0top1sz,omitempty"`
	X0       *int64   `json:"x0,omitempty"`
	X1       *int64   `json:"x1,omitempty"`
	X2       *int64   `json:"x2,omitempty"`
	Y0       *int64   `json:"y0,omitempty"`
	Y1       *int64   `json:"y1,omitempty"`
	Y2       *int64   `json:"y2,omitempty"`
}

// Classifier based trigger region; * dir: top-bottom, bottom-top, disabled
type TriggerRegion0 struct {
	Dir *string `json:"dir,omitempty"`
	X0  *int64  `json:"x0,omitempty"`
	X1  *int64  `json:"x1,omitempty"`
	Y0  *int64  `json:"y0,omitempty"`
	Y1  *int64  `json:"y1,omitempty"`
}

// AutoFocus configs
type AutoFocus struct {
	CoarseStep        *int64        `json:"coarseStep,omitempty"`
	ContrastThreshold *float64      `json:"contrastThreshold,omitempty"`
	Roi               *AutoFocusRoi `json:"roi,omitempty"`
	Run               *bool         `json:"run,omitempty"`
	UpdateRate        *int64        `json:"updateRate,omitempty"`
}

type AutoFocusRoi struct {
	CenterX *int64 `json:"centerX,omitempty"`
	CenterY *int64 `json:"centerY,omitempty"`
	Height  *int64 `json:"height,omitempty"`
	Width   *int64 `json:"width,omitempty"`
}

// Stream(s) configuration of device
type StreamConfig struct {
	H264  *H264  `json:"h264,omitempty"`
	Mjpeg *Mjpeg `json:"mjpeg,omitempty"`
}

type H264 struct {
	Available   *bool     `json:"available,omitempty"`
	EncoderType *string   `json:"encoder_type,omitempty"`
	Main        *H264Main `json:"main,omitempty"`
}

// Single h26x stream configuration
type H264Main struct {
	Available   *bool   `json:"available,omitempty"`
	Bitrate     *int64  `json:"bitrate,omitempty"`
	ControlRate *string `json:"controlRate,omitempty"`
	Enabled     *bool   `json:"enabled,omitempty"`
	Gop         *int64  `json:"gop,omitempty"`
	Profile     *string `json:"profile,omitempty"`
	Running     *bool   `json:"running,omitempty"`
	Source      *string `json:"source,omitempty"`
}

type Mjpeg struct {
	Available *bool      `json:"available,omitempty"`
	Main      *MjpegMain `json:"main,omitempty"`
}

// Single MJPEG stream configuration
type MjpegMain struct {
	Available *bool    `json:"available,omitempty"`
	Enabled   *bool    `json:"enabled,omitempty"`
	Framerate *float64 `json:"framerate,omitempty"`
	Quality   *int64   `json:"quality,omitempty"`
}

// Miscellaneous configs
type Misc struct {
	CameraOrientation        *bool          `json:"cameraOrientation,omitempty"`
	IrisHint                 *string        `json:"irisHint,omitempty"`
	JPEGQuality              *int64         `json:"jpegQuality,omitempty"`
	LegacyTsyncGpio          *int64         `json:"legacyTsyncGpio,omitempty"`
	Scenario1Crop            *Scenario1Crop `json:"scenario1Crop,omitempty"`
	Scenario1Overlay         *string        `json:"scenario1Overlay,omitempty"`
	Scenario1OverlayTextSize *int64         `json:"scenario1OverlayTextSize,omitempty"`
	Scenario2Crop            *Scenario2Crop `json:"scenario2Crop,omitempty"`
	Scenario2Overlay         *string        `json:"scenario2Overlay,omitempty"`
	Scenario2OverlayTextSize *int64         `json:"scenario2OverlayTextSize,omitempty"`
	ScenarioOverlayColor     *string        `json:"scenarioOverlayColor,omitempty"`
	SnapshotCrop             *SnapshotCrop  `json:"snapshotCrop,omitempty"`
}

type Scenario1Crop struct {
	X0 *int64 `json:"x0,omitempty"`
	X1 *int64 `json:"x1,omitempty"`
	Y0 *int64 `json:"y0,omitempty"`
	Y1 *int64 `json:"y1,omitempty"`
}

type Scenario2Crop struct {
	X0 *int64 `json:"x0,omitempty"`
	X1 *int64 `json:"x1,omitempty"`
	Y0 *int64 `json:"y0,omitempty"`
	Y1 *int64 `json:"y1,omitempty"`
}

type SnapshotCrop struct {
	Enable *bool   `json:"enable,omitempty"`
	Mode   *string `json:"mode,omitempty"`
	X0     *int64  `json:"x0,omitempty"`
	X1     *int64  `json:"x1,omitempty"`
	Y0     *int64  `json:"y0,omitempty"`
	Y1     *int64  `json:"y1,omitempty"`
}

// Current Miscellaneous Read-Only configs
type MiscVolatile struct {
	AE           *AE                `json:"ae,omitempty"`
	FPS          *FPS               `json:"fps,omitempty"`
	Gps          *Gps               `json:"gps,omitempty"`
	ISP          *ISP               `json:"isp,omitempty"`
	Lens         *MiscVolatileLens  `json:"lens,omitempty"`
	Profile      *Profile           `json:"profile,omitempty"`
	Whitebalance *WhitebalanceClass `json:"whitebalance,omitempty"`
}

type AE struct {
	CtrlMode *string  `json:"ctrlMode,omitempty"`
	LastRun  *int64   `json:"lastRun,omitempty"`
	Level    *float64 `json:"level,omitempty"`
}

type FPS struct {
	Mjpeg *float64 `json:"mjpeg,omitempty"`
}

type Gps struct {
	Altitude            *float64 `json:"altitude,omitempty"`
	Available           *bool    `json:"available,omitempty"`
	Bearing             *float64 `json:"bearing,omitempty"`
	Dop                 *float64 `json:"dop,omitempty"`
	Fix                 *string  `json:"fix,omitempty"`
	Latitude            *float64 `json:"latitude,omitempty"`
	Longitude           *float64 `json:"longitude,omitempty"`
	NumSatellites       *int64   `json:"numSatellites,omitempty"`
	SecondsSinceLastFix *int64   `json:"secondsSinceLastFix,omitempty"`
	Speed               *float64 `json:"speed,omitempty"`
	Time                *int64   `json:"time,omitempty"`
}

type ISP struct {
	FreeBuffers *int64  `json:"freeBuffers,omitempty"`
	Gain        *int64  `json:"gain,omitempty"`
	Iris        *int64  `json:"iris,omitempty"`
	IrisModel   *string `json:"irisModel,omitempty"`
	Shutter     *int64  `json:"shutter,omitempty"`
}

type MiscVolatileLens struct {
	Focus *int64 `json:"focus,omitempty"`
	Zoom  *int64 `json:"zoom,omitempty"`
}

type Profile struct {
	ID   *int64  `json:"id,omitempty"`
	Name *string `json:"name,omitempty"`
}

// ITSCAMPRO service configuration
type ItscamproConfig struct {
	Itscampro *Itscampro `json:"itscampro,omitempty"`
}

type Itscampro struct {
	Address *string `json:"address,omitempty"`
	Debug   *bool   `json:"debug,omitempty"`
	Enable  *bool   `json:"enable,omitempty"`
	Port    *int64  `json:"port,omitempty"`
}

// ITSCAMPRO service status
type ItscamproStatus struct {
	Status *string `json:"status,omitempty"`
}

// ImageSign service configuration
type ImageSignConfig struct {
	Sign *Sign `json:"sign,omitempty"`
}

type Sign struct {
	AppendMode *string `json:"appendMode,omitempty"`
	Enabled    *bool   `json:"enabled,omitempty"`
	Loaded     *bool   `json:"loaded,omitempty"`
	Update     *bool   `json:"update,omitempty"`
}

// FTP service configuration
type FTPConfig struct {
	FTP *FTP `json:"ftp,omitempty"`
}

type FTP struct {
	Address   *string   `json:"address,omitempty"`
	Anonymous *bool     `json:"anonymous,omitempty"`
	Enable    *bool     `json:"enable,omitempty"`
	Filename  *string   `json:"filename,omitempty"`
	Local     *Local    `json:"local,omitempty"`
	Password  *string   `json:"password,omitempty"`
	Port      *int64    `json:"port,omitempty"`
	Protocol  *string   `json:"protocol,omitempty"`
	Quality   *int64    `json:"quality,omitempty"`
	Transfer  *Transfer `json:"transfer,omitempty"`
	Username  *string   `json:"username,omitempty"`
}

type Local struct {
	BufferSizeKB *int64 `json:"bufferSizeKb,omitempty"`
	TTL          *int64 `json:"ttl,omitempty"`
}

type Transfer struct {
	PollInterval *int64 `json:"pollInterval,omitempty"`
	Timeout      *int64 `json:"timeout,omitempty"`
}

// Lince service configuration
type LinceConfig struct {
	AuthCode        *string `json:"authCode,omitempty"`
	ClientEndpoint  *string `json:"clientEndpoint,omitempty"`
	ClientID        *string `json:"clientId,omitempty"`
	Enabled         *bool   `json:"enabled,omitempty"`
	Environment     *string `json:"environment,omitempty"`
	SendRecsNone    *bool   `json:"sendRecsNone,omitempty"`
	TimeoutResponse *int64  `json:"timeoutResponse,omitempty"`
}

// Lince service status
type LinceStatus struct {
	LinceStatus *string `json:"linceStatus,omitempty"`
}

// VehicleIndicator service configuration
type VehicleIndicatorConfig struct {
	VehicleIndicator *VehicleIndicator `json:"vehicleIndicator,omitempty"`
}

type VehicleIndicator struct {
	VehicleCounterActiveHigh      *bool   `json:"vehicleCounterActiveHigh,omitempty"`
	VehicleCounterEnabled         *bool   `json:"vehicleCounterEnabled,omitempty"`
	VehicleCounterGpio            *int64  `json:"vehicleCounterGpio,omitempty"`
	VehicleCounterPulseWidthMS    *int64  `json:"vehicleCounterPulseWidthMs,omitempty"`
	VehicleCounterType            *int64  `json:"vehicleCounterType,omitempty"`
	VehicleCounterUDPPort         *int64  `json:"vehicleCounterUdpPort,omitempty"`
	VehicleCounterUDPSampleTimeMS *int64  `json:"vehicleCounterUdpSampleTimeMs,omitempty"`
	VehicleCounterUDPServer       *string `json:"vehicleCounterUdpServer,omitempty"`
}

// Protocols configurations
type ProtocolsConfig struct {
	ConfigCGI      *ConfigCGI      `json:"configCgi,omitempty"`
	Cougar         *Cougar         `json:"cougar,omitempty"`
	Itscamprotocol *Itscamprotocol `json:"itscamprotocol,omitempty"`
}

type ConfigCGI struct {
	BlockAPI *bool `json:"blockAPI,omitempty"`
}

type Cougar struct {
	Auth *Auth `json:"auth,omitempty"`
}

type Auth struct {
	Password *string `json:"password,omitempty"`
	Require  *bool   `json:"require,omitempty"`
}

type Itscamprotocol struct {
	LegacyMode *bool `json:"legacyMode,omitempty"`
}

// Profile Transitioner configs
type ProfileTransitioner struct {
	Automatic      *bool   `json:"automatic,omitempty"`
	LevelSmoothing *string `json:"levelSmoothing,omitempty"`
	ResetProfiles  *string `json:"resetProfiles,omitempty"`
	SmoothingTime  *int64  `json:"smoothingTime,omitempty"`
}

// Lanes configuration
type LanesConfig struct {
	Enabled *bool    `json:"enabled,omitempty"`
	Region0 *Region0 `json:"region0,omitempty"`
	Region1 *Region0 `json:"region1,omitempty"`
	Region2 *Region0 `json:"region2,omitempty"`
}

// Lane region
type Region0 struct {
	Name *string `json:"name,omitempty"`
	X0   *int64  `json:"x0,omitempty"`
	X1   *int64  `json:"x1,omitempty"`
	X2   *int64  `json:"x2,omitempty"`
	X3   *int64  `json:"x3,omitempty"`
	Y0   *int64  `json:"y0,omitempty"`
	Y1   *int64  `json:"y1,omitempty"`
	Y2   *int64  `json:"y2,omitempty"`
	Y3   *int64  `json:"y3,omitempty"`
}

// Configuration for a specific IO
type IoConfig struct {
	CanFlash   *bool   `json:"canFlash,omitempty"`
	CanTrigger *bool   `json:"canTrigger,omitempty"`
	EarlyUs    *int64  `json:"earlyUs,omitempty"`
	Group      *string `json:"group,omitempty"`
	Identifier *string `json:"identifier,omitempty"`
	IsInput    *bool   `json:"isInput,omitempty"`
	IsOn       *bool   `json:"isOn,omitempty"`
	Port       int64   `json:"port"`
	Protection *string `json:"protection,omitempty"`
	Type       *string `json:"type,omitempty"`
}

// Simplified information for a specific IO
type IoBasic struct {
	IsInput *bool `json:"isInput,omitempty"`
	IsOn    *bool `json:"isOn,omitempty"`
	Port    int64 `json:"port"`
}

// REST API Client service configuration
type RESTAPIClientConfig struct {
	Body                   Body        `json:"body"`
	Enabled                bool        `json:"enabled"`
	Headers                []Header    `json:"headers"`
	JPEG                   JPEG        `json:"jpeg"`
	Method                 Method      `json:"method"`
	Persistency            Persistency `json:"persistency"`
	Retries                int64       `json:"retries"`
	SendIndividualRequests bool        `json:"sendIndividualRequests"`
	SendWithoutOcr         bool        `json:"sendWithoutOcr"`
	Timeout                int64       `json:"timeout"`
	URL                    URL         `json:"url"`
}

type Body struct {
	Parts   []Part  `json:"parts"`
	Variant Variant `json:"variant"`
}

type Part struct {
	Content string `json:"content"`
	Name    string `json:"name"`
	Type    Type   `json:"type"`
}

type Header struct {
	Name  string `json:"name"`
	Value string `json:"value"`
}

type JPEG struct {
	Quality    int64      `json:"quality"`
	Resolution Resolution `json:"resolution"`
}

type Resolution struct {
	Height int64 `json:"height"`
	Width  int64 `json:"width"`
}

type Persistency struct {
	Enabled      bool  `json:"enabled"`
	MaxDiskUsage int64 `json:"maxDiskUsage"`
	MaxFileAge   int64 `json:"maxFileAge"`
	NewestFirst  bool  `json:"newestFirst"`
}

type URL struct {
	Host   string   `json:"host"`
	Path   string   `json:"path"`
	Query  []string `json:"query"`
	Scheme Scheme   `json:"scheme"`
}

// REST API Client service status
type RESTAPIClientStatus struct {
	Code      int64  `json:"code"`
	DiskUsage int64  `json:"diskUsage"`
	FileCount int64  `json:"fileCount"`
	Message   string `json:"message"`
}

// License service information
type Licenses struct {
	Analytics *Analytics `json:"analytics,omitempty"`
	DeviceID  *DeviceID  `json:"deviceId,omitempty"`
}

type Analytics struct {
	Classifier *AnalyticsClassifier `json:"classifier,omitempty"`
	Ocr        *AnalyticsOcr        `json:"ocr,omitempty"`
}

type AnalyticsClassifier struct {
	Customer       *string `json:"customer,omitempty"`
	MaxConnections *int64  `json:"maxConnections,omitempty"`
	MaxThreads     *int64  `json:"maxThreads,omitempty"`
	Serial         *string `json:"serial,omitempty"`
	Sha1           *string `json:"sha1,omitempty"`
	State          *int64  `json:"state,omitempty"`
	TTL            *int64  `json:"ttl,omitempty"`
	Version        *string `json:"version,omitempty"`
}

type AnalyticsOcr struct {
	Customer       *string `json:"customer,omitempty"`
	MaxConnections *int64  `json:"maxConnections,omitempty"`
	MaxThreads     *int64  `json:"maxThreads,omitempty"`
	Serial         *string `json:"serial,omitempty"`
	Sha1           *string `json:"sha1,omitempty"`
	State          *int64  `json:"state,omitempty"`
	TTL            *int64  `json:"ttl,omitempty"`
	Version        *string `json:"version,omitempty"`
}

type DeviceID struct {
	Serial *string `json:"serial,omitempty"`
}

type Mode string

const (
	Disabled Mode = "disabled"
	Fast     Mode = "fast"
	Normal   Mode = "normal"
	Slow     Mode = "slow"
)

type Type string

const (
	JSON Type = "json"
)

type Variant string

const (
	Multipart  Variant = "multipart"
	Singlepart Variant = "singlepart"
)

type Method string

const (
	Get  Method = "get"
	Post Method = "post"
	Put  Method = "put"
)

type Scheme string

const (
	HTTP  Scheme = "http"
	HTTPS Scheme = "https"
)
