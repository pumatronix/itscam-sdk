// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// TypeScript ambient module declaration for @pumatronix/itscam-sdk.

declare module '@pumatronix/itscam-sdk' {
    // -----------------------------------------------------------------
    //  Errors
    // -----------------------------------------------------------------

    export const ErrorCode: {
        readonly OK: 0;
        readonly CONNECTION_FAILED: 1;
        readonly TIMEOUT: 2;
        readonly NOT_AUTHENTICATED: 3;
        readonly INVALID_PARAMETER: 4;
        readonly SERVER_ERROR: 5;
        readonly DISCONNECTED: 6;
        readonly UNKNOWN: 7;
        readonly NULL_HANDLE: 8;
        readonly ALLOCATION_FAILED: 9;
    };

    export class ItscamError extends Error {
        readonly code: number;
        readonly codeName: string;
    }
    export class ItscamTimeoutError extends ItscamError {}
    export class ItscamAuthError extends ItscamError {}
    export class ItscamConnectionError extends ItscamError {}
    export class ItscamInvalidParameterError extends ItscamError {}
    export class ItscamServerError extends ItscamError {}

    // -----------------------------------------------------------------
    //  Common types
    // -----------------------------------------------------------------

    export interface Timestamp {
        year: number;
        month: number;
        day: number;
        hour: number;
        minute: number;
        second: number;
        millisecond: number;
        timezoneOffset: number;
    }

    export interface FrameInfo {
        requestId: number;
        frameCount: number;
        multiExpIndex: number;
        multiExpLength: number;
        shutter: number;
        gain: number;
        width: number;
        height: number;
        timestamp: Timestamp;
        plates: string[];
    }

    export interface CaptureResult {
        info: FrameInfo;
        plates: string[];
        jpeg: Buffer;
    }

    export interface ProfileInfo {
        id: number;
        name: string;
        description: string;
        active: boolean;
    }

    export interface AutoReconnectConfig {
        enabled?: boolean;
        intervalMs?: number;
        maxRetries?: number;
    }

    export interface CaptureSubscriptionConfig {
        includeTrigger?: boolean;
        includeSnapshot?: boolean;
        includeMetadata?: boolean;
        embedComments?: boolean;
        embedExif?: boolean;
        embedSignature?: boolean;
        triggerQuality?: number;
        snapshotQuality?: number;
    }

    export interface EventSubscription {
        pipeline?: boolean;
        triggerMetadata?: boolean;
        triggerImage?: boolean;
        snapshotMetadata?: boolean;
        snapshotImage?: boolean;
        previewMetadata?: boolean;
        previewImage?: boolean;
        gpio?: boolean;
        serial1?: boolean;
        serial2?: boolean;
    }

    export interface CgiImage {
        mimeType: string;
        data: Buffer;
    }

    export interface CgiStreamFrame {
        sequence: number;
        mimeType: string;
        data: Buffer;
    }

    export interface SnapshotCgiRequest {
        shutters?: number[];
        gains?: number[];
        quality?: number;
        mosaic?: boolean;
        format?: string;
        scenario?: number;
        crop?: string;
        textOverlay?: string;
        userMetadata?: Record<string, string>;
    }

    export const ConnectionState: {
        readonly CONNECTED: 0;
        readonly DISCONNECTED: 1;
        readonly RECONNECTING: 2;
        readonly RECONNECTED: 3;
    };
    export const LogLevel: {
        readonly INFO: 0;
        readonly ERROR: 1;
    };

    // -----------------------------------------------------------------
    //  Binary client
    // -----------------------------------------------------------------

    export class ItscamClient {
        constructor();
        close(): void;

        connect(address: string, port?: number, timeoutMs?: number,
                reconnect?: AutoReconnectConfig | null): void;
        connectAsync(address: string, port?: number, timeoutMs?: number,
                     reconnect?: AutoReconnectConfig | null): Promise<void>;
        disconnect(): void;
        isConnected(): boolean;

        authenticate(password: string, timeoutMs?: number): void;
        authenticateAsync(password: string, timeoutMs?: number): Promise<void>;

        subscribe(events: EventSubscription, timeoutMs?: number): void;
        subscribeCaptures(config?: CaptureSubscriptionConfig,
                          timeoutMs?: number): void;

        captureSnapshot(timeoutMs?: number): CaptureResult[];
        captureSnapshotAsync(timeoutMs?: number): Promise<CaptureResult[]>;

        getLastFrame(quality?: number, timeoutMs?: number): Buffer;

        getActiveProfileId(timeoutMs?: number): number;
        setActiveProfile(profileId: number, timeoutMs?: number): void;
        listProfiles(timeoutMs?: number): ProfileInfo[];
        reboot(timeoutMs?: number): void;

        onTriggerImage(callback: ((r: CaptureResult) => void) | null): void;
        onSnapshotImage(callback: ((r: CaptureResult) => void) | null): void;
        onDisconnect(callback: ((reason: string) => void) | null): void;
        onConnectionState(
            callback: ((state: number, reason: string) => void) | null): void;
        onLog(callback: ((level: number, message: string) => void) | null): void;
    }

    // -----------------------------------------------------------------
    //  REST client
    // -----------------------------------------------------------------

    export type JsonValue = string | number | boolean | null
        | { [k: string]: JsonValue } | JsonValue[];

    export class ItscamRestClient {
        constructor();
        close(): void;

        setBaseUrl(host: string, port?: number, scheme?: string): void;
        setApiPrefix(prefix: string): void;
        setCaCertFile(pemPath: string): void;
        setCaCertData(pem: string): void;
        setVerifyServerCertificate(verify: boolean): void;
        setClientCertificate(certPem: string, keyPem: string): void;

        login(username: string, password: string,
              timeoutMs?: number): JsonValue;
        loginAsync(username: string, password: string,
                   timeoutMs?: number): Promise<JsonValue>;
        setAuthToken(token: string): void;
        clearAuthToken(): void;

        get(path: string, timeoutMs?: number): JsonValue;
        put(path: string, body: JsonValue, timeoutMs?: number): JsonValue;
        post(path: string, body: JsonValue, timeoutMs?: number): JsonValue;
        delete(path: string, timeoutMs?: number): JsonValue;
        patchJson(path: string, body: JsonValue, timeoutMs?: number): JsonValue;
        getAsync(path: string, timeoutMs?: number): Promise<JsonValue>;
        putAsync(path: string, body: JsonValue, timeoutMs?: number): Promise<JsonValue>;
        postAsync(path: string, body: JsonValue, timeoutMs?: number): Promise<JsonValue>;
        deleteAsync(path: string, timeoutMs?: number): Promise<JsonValue>;

        // Typed helpers (raw JSON)
        getProfiles(timeoutMs?: number): JsonValue;
        getProfile(profileId: number, timeoutMs?: number): JsonValue;
        createProfile(profile: JsonValue, timeoutMs?: number): JsonValue;
        updateProfile(profile: JsonValue, timeoutMs?: number): JsonValue;
        deleteProfile(profileId: number, timeoutMs?: number): JsonValue;

        getVolatileInfo(timeoutMs?: number): JsonValue;
        getOcrConfig(timeoutMs?: number): JsonValue;
        setOcrConfig(cfg: JsonValue, timeoutMs?: number): JsonValue;
        getAnalyticsConfig(timeoutMs?: number): JsonValue;
        setAnalyticsConfig(cfg: JsonValue, timeoutMs?: number): JsonValue;
        getClassifierConfig(timeoutMs?: number): JsonValue;
        setClassifierConfig(cfg: JsonValue, timeoutMs?: number): JsonValue;
        getLanesConfig(timeoutMs?: number): JsonValue;
        setLanesConfig(cfg: JsonValue, timeoutMs?: number): JsonValue;
        getItscamproConfig(timeoutMs?: number): JsonValue;
        setItscamproConfig(cfg: JsonValue, timeoutMs?: number): JsonValue;
        getItscamproStatus(timeoutMs?: number): JsonValue;
    }

    // -----------------------------------------------------------------
    //  CGI client
    // -----------------------------------------------------------------

    export class ItscamCgiClient {
        constructor();
        close(): void;

        setBaseUrl(host: string, port?: number, scheme?: string): void;
        setApiPrefix(prefix: string): void;
        setCaCertFile(pemPath: string): void;
        setCaCertData(pem: string): void;
        setVerifyServerCertificate(verify: boolean): void;
        setClientCertificate(certPem: string, keyPem: string): void;

        login(username: string, password: string, timeoutMs?: number): void;
        loginAsync(username: string, password: string,
                   timeoutMs?: number): Promise<void>;
        setAuthToken(token: string): void;
        clearAuthToken(): void;
        setBasicAuth(user: string, password: string): void;
        clearBasicAuth(): void;

        getLastFrame(timeoutMs?: number): CgiImage;
        getLastFrameAsync(timeoutMs?: number): Promise<CgiImage>;

        getSnapshot(request?: SnapshotCgiRequest,
                    timeoutMs?: number): CgiImage[];
        getSnapshotAsync(request?: SnapshotCgiRequest,
                         timeoutMs?: number): Promise<CgiImage[]>;

        startMjpegStream(onFrame: (frame: CgiStreamFrame) => void,
                         timeoutMs?: number): void;
        stopMjpegStream(): void;
        isMjpegStreamRunning(): boolean;

        forceTrigger(timeoutMs?: number): string;
        reboot(timeoutMs?: number): string;
    }

    // -----------------------------------------------------------------
    //  JPEG metadata helpers
    // -----------------------------------------------------------------

    export interface PlateRecognition {
        plate: string;
        x: number;
        y: number;
        width: number;
        height: number;
        color: string;
        countryCode: number;
    }

    export interface ObjectDetection {
        type: number;
        probability: number;
        x: number; y: number; width: number; height: number;
        brand: string; brandProb: number;
        model: string; modelProb: number;
        color: string; colorProb: number;
    }

    export interface JpegMetadata {
        comment: string;
        tags: Record<string, string>;
        plates: PlateRecognition[];
        objects: ObjectDetection[];
    }

    export function extractJpegComment(jpeg: Buffer | Uint8Array): string;
    export function parseJpegCommentTags(comment: string): Record<string, string>;
    export function extractPlateRecognitions(
        tags: Record<string, string>): PlateRecognition[];
    export function extractObjectDetections(
        tags: Record<string, string>): ObjectDetection[];
    export function parseJpegMetadata(jpeg: Buffer | Uint8Array): JpegMetadata;

    // -----------------------------------------------------------------
    //  Misc
    // -----------------------------------------------------------------

    export function getNativeLibraryVersion(): string;
    export function getWrapperVersion(): string;
    export function getWrapperVersionFull(): string;
    export function getLibraryPath(): string;
    export function getSystemLocalTime(): Timestamp;
    export function getSystemUtcTime(): Timestamp;
    export function getEpochTime(): number;
    export function getEpochTimeMs(): number;
    export function getLastError(): string;
    export const VERSION: string;
    export const VERSION_FULL: string;
}
