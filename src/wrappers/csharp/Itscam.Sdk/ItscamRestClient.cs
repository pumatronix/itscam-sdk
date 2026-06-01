// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using Pumatronix.Itscam.Native;
using Pumatronix.Itscam.RestTypes;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// .NET wrapper for the ITSCAM REST API (webapp backend).
    ///
    /// Two coexisting surfaces:
    ///
    ///   * <b>Typed helpers</b> (preferred) -- e.g. <see cref="GetOcrConfigAsync(uint)"/>,
    ///     return strongly-typed POCOs generated from the camera's OpenAPI
    ///     document.  See <c>tools/codegen/</c> for how to regenerate them
    ///     against a newer firmware.
    ///   * <b>Generic verbs</b> (escape hatch) -- <see cref="GetAsync(string, uint)"/>,
    ///     <see cref="PutAsync(string, string, uint)"/> etc. return the
    ///     server's JSON as a UTF-8 string.  Use these for endpoints that are
    ///     not (yet) typed or to ship partial-update bodies that would lose
    ///     fidelity when round-tripped through a POCO.
    ///
    /// Set <see cref="SetBaseUrl(string, ushort, string)"/>'s scheme to
    /// "https" to use TLS; certificates are configured through the
    /// SetCaCert* / SetVerifyServerCertificate / SetClientCertificate
    /// methods.
    /// </summary>
    public sealed class ItscamRestClient : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;

        public ItscamRestClient()
        {
            _handle = NativeMethods.Rest_create();
            if (_handle == IntPtr.Zero)
                throw new ItscamException(ItscamErrorCode.AllocationFailed,
                    "Failed to allocate native REST client.");
        }

        // ====================================================================
        //  Configuration
        // ====================================================================

        /// <summary>
        /// Configure host, port and scheme.  Pass scheme = "https" for TLS;
        /// pass port = 0 to use the protocol default (80 or 443).
        /// </summary>
        public void SetBaseUrl(string host, ushort port = 80,
                               string scheme = "http")
        {
            ThrowIfDisposed();
            if (host == null) throw new ArgumentNullException(nameof(host));
            var rc = NativeMethods.Rest_setBaseUrl(_handle, host, port,
                                                   scheme ?? "http");
            ItscamException.ThrowIfFailed(rc, "Rest_setBaseUrl");
        }

        public void SetCaCertFile(string pemPath)
        {
            ThrowIfDisposed();
            NativeMethods.Rest_setCaCertFile(_handle,
                                             pemPath ?? string.Empty);
        }

        public void SetCaCertData(string pem)
        {
            ThrowIfDisposed();
            NativeMethods.Rest_setCaCertData(_handle, pem ?? string.Empty);
        }

        public void SetVerifyServerCertificate(bool verify)
        {
            ThrowIfDisposed();
            NativeMethods.Rest_setVerifyServerCertificate(_handle,
                                                          verify ? 1 : 0);
        }

        public void SetClientCertificate(string certPem, string keyPem)
        {
            ThrowIfDisposed();
            NativeMethods.Rest_setClientCertificate(
                _handle, certPem ?? string.Empty, keyPem ?? string.Empty);
        }

        // ====================================================================
        //  Authentication
        // ====================================================================

        /// <summary>
        /// POST /api/auth, store the returned bearer token internally and
        /// return the JSON response from the server.
        /// </summary>
        public Task<string> LoginAsync(string username, string password,
                                       uint timeoutMs = 10000)
        {
            ThrowIfDisposed();
            if (username == null) throw new ArgumentNullException(nameof(username));
            if (password == null) throw new ArgumentNullException(nameof(password));
            return Task.Run(() =>
            {
                var rc = NativeMethods.Rest_login(_handle, username, password,
                                                  timeoutMs, out var ptr);
                if (rc != NativeErrorCode.Ok)
                {
                    if (ptr != IntPtr.Zero) NativeMethods.String_destroy(ptr);
                    ItscamException.ThrowIfFailed(rc, "Rest.login");
                }
                return NativeMethods.TakeString(ptr);
            });
        }

        public void SetAuthToken(string token)
        {
            ThrowIfDisposed();
            NativeMethods.Rest_setAuthToken(_handle, token ?? string.Empty);
        }

        public void ClearAuthToken()
        {
            ThrowIfDisposed();
            NativeMethods.Rest_clearAuthToken(_handle);
        }

        // ====================================================================
        //  HTTP verbs
        // ====================================================================

        public Task<string> GetAsync(string path, uint timeoutMs = 10000) =>
            InvokeAsync("GET", path, null, timeoutMs, NativeMethods.Rest_httpGet);

        public Task<string> PutAsync(string path, string jsonBody,
                                     uint timeoutMs = 10000) =>
            InvokeAsync("PUT", path, jsonBody, timeoutMs, null);

        public Task<string> PostAsync(string path, string jsonBody,
                                      uint timeoutMs = 10000) =>
            InvokeAsync("POST", path, jsonBody, timeoutMs, null);

        public Task<string> DeleteAsync(string path, uint timeoutMs = 10000) =>
            InvokeAsync("DELETE", path, null, timeoutMs,
                        NativeMethods.Rest_httpDelete);

        // ====================================================================
        //  JSON patch helpers
        // ====================================================================

        /// <summary>
        /// PUT a partial JSON document to <paramref name="path"/>.
        ///
        /// <para>
        /// Typed setters already use partial serialization (null properties
        /// are omitted), so <see cref="PatchJsonAsync"/> is mainly useful for
        /// endpoints without a typed helper or for hand-built patches.
        /// </para>
        ///
        /// <para>
        /// Example -- disable the hardware trigger on profile 0:
        /// <code>
        /// // Preferred typed approach:
        /// await rest.UpdateProfileByIdAsync(0,
        ///     new ProfileConfig { Trigger = new Trigger { Enabled = false } });
        ///
        /// // Untyped alternative:
        /// await rest.PatchJsonAsync("/api/image/profiles/0",
        ///     new JsonObject { ["trigger"] = new JsonObject { ["enabled"] = false } });
        /// </code>
        /// </para>
        /// </summary>
        public Task<string> PatchJsonAsync(string path, JsonNode patch,
                                           uint timeoutMs = 10000)
        {
            if (path == null)  throw new ArgumentNullException(nameof(path));
            if (patch == null) throw new ArgumentNullException(nameof(patch));
            return PutAsync(path, patch.ToJsonString(), timeoutMs);
        }

        /// <summary>
        /// GET the resource at <paramref name="path"/> as raw JSON, hand the
        /// parsed <see cref="JsonNode"/> to <paramref name="mutate"/> to be
        /// edited in place, and PUT the <b>entire</b> result back.
        ///
        /// <para>
        /// <b>Warning:</b> many ITSCAM endpoints (including
        /// <c>PUT /api/image/profiles/{id}</c>) reject a full-document PUT
        /// with HTTP 500 even when the body is an unmodified GET response.
        /// For configuration changes prefer <see cref="PatchJsonAsync"/> to
        /// send only the fields you want to change.
        /// </para>
        /// </summary>
        /// <param name="path">Absolute REST path, e.g. "/api/image/profiles/0".</param>
        /// <param name="mutate">
        /// Action that edits the parsed JSON tree in place.  Called exactly
        /// once with the root <see cref="JsonNode"/> of the GET response.
        /// </param>
        /// <param name="timeoutMs">
        /// Timeout (in milliseconds) applied to both the GET and the PUT.
        /// </param>
        /// <returns>Server response from the PUT, as raw JSON.</returns>
        public async Task<string> UpdateJsonAsync(string path,
                                                  Action<JsonNode> mutate,
                                                  uint timeoutMs = 10000)
        {
            if (path == null)   throw new ArgumentNullException(nameof(path));
            if (mutate == null) throw new ArgumentNullException(nameof(mutate));

            string currentJson = await GetAsync(path, timeoutMs)
                .ConfigureAwait(false);
            JsonNode root = JsonNode.Parse(currentJson);
            if (root == null)
                throw new ItscamException(ItscamErrorCode.InvalidParameter,
                    $"GET {path} did not return a JSON document.");

            mutate(root);

            return await PutAsync(path, root.ToJsonString(), timeoutMs)
                .ConfigureAwait(false);
        }

        // ====================================================================
        //  Typed convenience helpers
        //
        //  These thin overloads round-trip JSON through the auto-generated
        //  Pumatronix.Itscam.RestTypes POCOs.  The native C ABI continues to
        //  carry JSON strings unchanged.
        //
        //  IMPORTANT -- These are LOSSY round-trips.  System.Text.Json
        //  silently drops every JSON property that does not appear in the
        //  generated POCO, so the SetXAsync(GetXAsync()) pattern will strip
        //  fields the camera serialises but our snapshot of the OpenAPI
        //  document does not yet document.  Many daemon endpoints (notably
        //  PUT /api/image/profiles/{id}) reject incomplete bodies with HTTP
        //  500.
        //
        //  Use the typed helpers for:
        //    * Reading a configuration to inspect specific fields.
        //    * Writing a configuration that the caller constructs from
        //      scratch.
        //
        //  For partial updates (the common "tweak one field on an existing
        //  config" workflow) use <see cref="UpdateJsonAsync"/> instead.  It
        //  preserves every byte of the camera's GET response except the
        //  fields the caller explicitly mutates.
        // ====================================================================

        // Tuned to mirror the C++ side: ignore unknown fields, write null
        // fields lazily, kebab/snake/Pascal names are handled by the
        // [JsonPropertyName] attributes the generator emits.
        internal static readonly JsonSerializerOptions JsonOpts =
            new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true,
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                ReadCommentHandling = JsonCommentHandling.Skip,
                AllowTrailingCommas = true,
            };

        private static T Deserialize<T>(string json, string context)
        {
            if (string.IsNullOrEmpty(json))
                throw new ItscamServerException(
                    $"Empty server response for {context}.");
            try
            {
                var v = JsonSerializer.Deserialize<T>(json, JsonOpts);
                if (v == null)
                    throw new ItscamServerException(
                        $"Null JSON payload deserialising {context}.");
                return v;
            }
            catch (JsonException ex)
            {
                // Surface the offending JSON fragment so callers can identify
                // schema mismatches without having to enable wire-level
                // logging.  The generated typed wrappers come from a snapshot
                // of itscam600's OpenAPI document and the upstream JSDoc
                // occasionally misdeclares numeric fields; see
                // tools/codegen/postprocess.mjs::applyUpstreamPatches.
                string snippet = ExtractJsonSnippet(json, ex);
                throw new ItscamException(ItscamErrorCode.InvalidParameter,
                    $"Schema mismatch for {context}: {ex.Message}"
                    + (snippet != null ? $"\nOffending JSON: {snippet}" : ""));
            }
        }

        /// <summary>
        /// Extract a short window of the JSON payload around the byte position
        /// reported by <see cref="JsonException"/> so error messages point at
        /// the specific value that failed to deserialise.
        /// </summary>
        private static string ExtractJsonSnippet(string json, JsonException ex)
        {
            if (json == null || ex.BytePositionInLine == null)
                return null;
            int pos = (int)Math.Min((long)ex.BytePositionInLine.Value,
                                    json.Length - 1);
            int start = Math.Max(0, pos - 60);
            int end = Math.Min(json.Length, pos + 60);
            string before = json.Substring(start, pos - start);
            string after = json.Substring(pos, end - pos);
            return $"...{before}>>>{after}...";
        }

        private static string Serialize<T>(T value) =>
            JsonSerializer.Serialize(value, JsonOpts);

        // ---- Image profiles ------------------------------------------------

        /// <summary>GET /api/image/profiles -- list all profiles.</summary>
        public async Task<List<ProfileConfig>> GetProfilesAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/image/profiles", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<List<ProfileConfig>>(json, "GET /image/profiles");
        }

        /// <summary>GET /api/image/profiles?id=&lt;id&gt;.</summary>
        public async Task<List<ProfileConfig>> GetProfileAsync(
            int id, uint timeoutMs = 10000)
        {
            string json = await GetAsync($"/api/image/profiles?id={id}",
                                         timeoutMs).ConfigureAwait(false);
            return Deserialize<List<ProfileConfig>>(
                json, $"GET /image/profiles?id={id}");
        }

        /// <summary>
        /// Get a single profile by name (case-sensitive match).
        /// </summary>
        /// <exception cref="ItscamInvalidParameterException">
        /// Thrown if no profile with the given name exists.
        /// </exception>
        public async Task<ProfileConfig> GetProfileByNameAsync(
            string name, uint timeoutMs = 10000)
        {
            if (name == null)
                throw new ArgumentNullException(nameof(name));
            var all = await GetProfilesAsync(timeoutMs).ConfigureAwait(false);
            var match = all.FirstOrDefault(p => p.Name == name);
            if (match == null)
                throw new ItscamInvalidParameterException(
                    $"profile not found: \"{name}\"");
            return match;
        }

        /// <summary>POST /api/image/profiles -- create a new profile.</summary>
        public async Task<ProfileConfig> CreateProfileAsync(
            ProfileConfig profile, uint timeoutMs = 10000)
        {
            if (profile == null)
                throw new ArgumentNullException(nameof(profile));
            string json = await PostAsync("/api/image/profiles",
                                          Serialize(profile), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ProfileConfig>(json, "POST /image/profiles");
        }

        /// <summary>
        /// PUT /api/image/profiles/{id} -- update a single profile.
        ///
        /// <para>
        /// Only non-null properties are included in the PUT body (partial
        /// serialization).  Construct a <see cref="ProfileConfig"/> with only
        /// the properties you want to change.
        /// </para>
        /// </summary>
        public async Task<ProfileConfig> UpdateProfileByIdAsync(
            int id, ProfileConfig profile, uint timeoutMs = 10000)
        {
            if (profile == null)
                throw new ArgumentNullException(nameof(profile));
            string json = await PutAsync($"/api/image/profiles/{id}",
                                         Serialize(profile), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ProfileConfig>(
                json, $"PUT /image/profiles/{id}");
        }

        /// <summary>
        /// Update a profile found by name.  Looks up the profile, then PUTs
        /// the partial update to its id.
        /// </summary>
        /// <exception cref="ItscamInvalidParameterException">
        /// Thrown if no profile with the given name exists.
        /// </exception>
        public async Task<ProfileConfig> UpdateProfileByNameAsync(
            string name, ProfileConfig profile, uint timeoutMs = 10000)
        {
            if (name == null)
                throw new ArgumentNullException(nameof(name));
            if (profile == null)
                throw new ArgumentNullException(nameof(profile));
            var found = await GetProfileByNameAsync(name, timeoutMs)
                .ConfigureAwait(false);
            return await UpdateProfileByIdAsync((int)found.Id, profile,
                                               timeoutMs)
                .ConfigureAwait(false);
        }

        /// <summary>
        /// PUT /api/image/profiles -- bulk update (JSON array body).
        ///
        /// <para>
        /// Each profile is partially serialized (null properties omitted).
        /// </para>
        /// </summary>
        public async Task<ProfileConfig> UpdateProfilesAsync(
            IEnumerable<ProfileConfig> profiles, uint timeoutMs = 10000)
        {
            if (profiles == null)
                throw new ArgumentNullException(nameof(profiles));
            string json = await PutAsync("/api/image/profiles",
                                         Serialize(profiles), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ProfileConfig>(json, "PUT /image/profiles");
        }

        // ---- Equipment volatile info --------------------------------------

        /// <summary>GET /api/equipment/misc/readonly/volatile.</summary>
        public async Task<MiscVolatile> GetVolatileInfoAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                "/api/equipment/misc/readonly/volatile", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<MiscVolatile>(json, "GET /equipment/misc/volatile");
        }

        // ---- OCR ----------------------------------------------------------

        /// <summary>GET /api/equipment/ocr.</summary>
        public async Task<OcrConfig> GetOcrConfigAsync(uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/ocr", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<OcrConfig>(json, "GET /equipment/ocr");
        }

        /// <summary>PUT /api/equipment/ocr.</summary>
        public async Task<OcrConfig> SetOcrConfigAsync(
            OcrConfig config, uint timeoutMs = 10000)
        {
            if (config == null)
                throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/ocr",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<OcrConfig>(json, "PUT /equipment/ocr");
        }

        // ---- Analytics ----------------------------------------------------

        /// <summary>GET /api/equipment/analytics.</summary>
        public async Task<AnalyticsConfig> GetAnalyticsConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/analytics", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<AnalyticsConfig>(
                json, "GET /equipment/analytics");
        }

        /// <summary>PUT /api/equipment/analytics.</summary>
        public async Task<AnalyticsConfig> SetAnalyticsConfigAsync(
            AnalyticsConfig config, uint timeoutMs = 10000)
        {
            if (config == null)
                throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/analytics",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<AnalyticsConfig>(
                json, "PUT /equipment/analytics");
        }

        // ---- Classifier ---------------------------------------------------

        /// <summary>GET /api/equipment/classifier.</summary>
        public async Task<ClassifierConfig> GetClassifierConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/classifier", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ClassifierConfig>(
                json, "GET /equipment/classifier");
        }

        /// <summary>PUT /api/equipment/classifier.</summary>
        public async Task<ClassifierConfig> SetClassifierConfigAsync(
            ClassifierConfig config, uint timeoutMs = 10000)
        {
            if (config == null)
                throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/classifier",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ClassifierConfig>(
                json, "PUT /equipment/classifier");
        }

        // ---- ITSCAM PRO server --------------------------------------------

        /// <summary>GET /api/equipment/servers/itscampro.</summary>
        public async Task<ItscamproConfig> GetItscamproConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                "/api/equipment/servers/itscampro", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ItscamproConfig>(
                json, "GET /equipment/servers/itscampro");
        }

        /// <summary>PUT /api/equipment/servers/itscampro.</summary>
        public async Task<ItscamproConfig> SetItscamproConfigAsync(
            ItscamproConfig config, uint timeoutMs = 10000)
        {
            if (config == null)
                throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/servers/itscampro",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ItscamproConfig>(
                json, "PUT /equipment/servers/itscampro");
        }

        /// <summary>GET /api/equipment/servers/itscampro/status.</summary>
        public async Task<ItscamproStatus> GetItscamproStatusAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                "/api/equipment/servers/itscampro/status", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ItscamproStatus>(
                json, "GET /equipment/servers/itscampro/status");
        }

        // ---- AutoFocus ----------------------------------------------------

        /// <summary>GET /api/equipment/autofocus.</summary>
        public async Task<AutoFocus> GetAutoFocusAsync(uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/autofocus", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<AutoFocus>(json, "GET /equipment/autofocus");
        }

        /// <summary>PUT /api/equipment/autofocus.</summary>
        public async Task<AutoFocus> SetAutoFocusAsync(
            AutoFocus config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/autofocus",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<AutoFocus>(json, "PUT /equipment/autofocus");
        }

        // ---- Stream config -------------------------------------------------

        /// <summary>GET /api/video/streams.</summary>
        public async Task<StreamConfig> GetStreamConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/video/streams", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<StreamConfig>(json, "GET /video/streams");
        }

        /// <summary>PUT /api/video/streams.</summary>
        public async Task<StreamConfig> SetStreamConfigAsync(
            StreamConfig config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/video/streams",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<StreamConfig>(json, "PUT /video/streams");
        }

        // ---- Equipment misc -----------------------------------------------

        /// <summary>GET /api/equipment/misc.</summary>
        public async Task<Misc> GetMiscAsync(uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/misc", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<Misc>(json, "GET /equipment/misc");
        }

        /// <summary>PUT /api/equipment/misc.</summary>
        public async Task<Misc> SetMiscAsync(Misc config,
                                             uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/misc",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<Misc>(json, "PUT /equipment/misc");
        }

        // ---- Image sign ----------------------------------------------------

        /// <summary>GET /api/equipment/imageSign.</summary>
        public async Task<ImageSignConfig> GetImageSignConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/imageSign", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ImageSignConfig>(
                json, "GET /equipment/imageSign");
        }

        // ---- FTP -----------------------------------------------------------

        /// <summary>GET /api/equipment/servers/ftp.</summary>
        public async Task<FtpConfig> GetFtpConfigAsync(uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/servers/ftp",
                                         timeoutMs).ConfigureAwait(false);
            return Deserialize<FtpConfig>(json, "GET /equipment/servers/ftp");
        }

        /// <summary>PUT /api/equipment/servers/ftp.</summary>
        public async Task<FtpConfig> SetFtpConfigAsync(
            FtpConfig config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/servers/ftp",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<FtpConfig>(json, "PUT /equipment/servers/ftp");
        }

        // ---- Lince ---------------------------------------------------------

        /// <summary>GET /api/equipment/servers/lince.</summary>
        public async Task<LinceConfig> GetLinceConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/servers/lince",
                                         timeoutMs).ConfigureAwait(false);
            return Deserialize<LinceConfig>(
                json, "GET /equipment/servers/lince");
        }

        /// <summary>PUT /api/equipment/servers/lince.</summary>
        public async Task<LinceConfig> SetLinceConfigAsync(
            LinceConfig config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/servers/lince",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<LinceConfig>(
                json, "PUT /equipment/servers/lince");
        }

        /// <summary>GET /api/equipment/servers/lince/status.</summary>
        public async Task<LinceStatus> GetLinceStatusAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                "/api/equipment/servers/lince/status", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<LinceStatus>(
                json, "GET /equipment/servers/lince/status");
        }

        // ---- Vehicle indicator --------------------------------------------

        /// <summary>GET /api/equipment/vehicleIndicator.</summary>
        public async Task<VehicleIndicatorConfig>
            GetVehicleIndicatorConfigAsync(uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                "/api/equipment/vehicleIndicator", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<VehicleIndicatorConfig>(
                json, "GET /equipment/vehicleIndicator");
        }

        /// <summary>PUT /api/equipment/vehicleIndicator.</summary>
        public async Task<VehicleIndicatorConfig>
            SetVehicleIndicatorConfigAsync(VehicleIndicatorConfig config,
                                           uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/vehicleIndicator",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<VehicleIndicatorConfig>(
                json, "PUT /equipment/vehicleIndicator");
        }

        // ---- Protocols -----------------------------------------------------

        /// <summary>GET /api/equipment/servers/protocols.</summary>
        public async Task<ProtocolsConfig> GetProtocolsConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/servers/protocols",
                                         timeoutMs).ConfigureAwait(false);
            return Deserialize<ProtocolsConfig>(
                json, "GET /equipment/servers/protocols");
        }

        /// <summary>PUT /api/equipment/servers/protocols.</summary>
        public async Task<ProtocolsConfig> SetProtocolsConfigAsync(
            ProtocolsConfig config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/servers/protocols",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ProtocolsConfig>(
                json, "PUT /equipment/servers/protocols");
        }

        // ---- Profile transitioner -----------------------------------------

        /// <summary>GET /api/equipment/transitioner.</summary>
        public async Task<ProfileTransitioner> GetProfileTransitionerAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/transitioner",
                                         timeoutMs).ConfigureAwait(false);
            return Deserialize<ProfileTransitioner>(
                json, "GET /equipment/transitioner");
        }

        /// <summary>PUT /api/equipment/transitioner.</summary>
        public async Task<ProfileTransitioner> SetProfileTransitionerAsync(
            ProfileTransitioner config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/transitioner",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<ProfileTransitioner>(
                json, "PUT /equipment/transitioner");
        }

        // ---- Lanes ---------------------------------------------------------

        /// <summary>GET /api/equipment/lanes.</summary>
        public async Task<LanesConfig> GetLanesConfigAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/lanes", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<LanesConfig>(json, "GET /equipment/lanes");
        }

        /// <summary>PUT /api/equipment/lanes.</summary>
        public async Task<LanesConfig> SetLanesConfigAsync(
            LanesConfig config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync("/api/equipment/lanes",
                                         Serialize(config), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<LanesConfig>(json, "PUT /equipment/lanes");
        }

        // ---- I/O ports -----------------------------------------------------

        /// <summary>GET /api/equipment/ioPorts (full pin array).</summary>
        public async Task<List<IoConfig>> GetIoPortsAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/ioPorts", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<List<IoConfig>>(json, "GET /equipment/ioPorts");
        }

        /// <summary>PUT /api/equipment/ioPorts (bulk update).</summary>
        public async Task<List<IoConfig>> SetIoPortsAsync(
            IEnumerable<IoConfig> ports, uint timeoutMs = 10000)
        {
            if (ports == null) throw new ArgumentNullException(nameof(ports));
            string json = await PutAsync("/api/equipment/ioPorts",
                                         Serialize(ports), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<List<IoConfig>>(json, "PUT /equipment/ioPorts");
        }

        /// <summary>GET /api/equipment/ioPorts/{id}.</summary>
        public async Task<IoConfig> GetIoPortAsync(int id,
                                                   uint timeoutMs = 10000)
        {
            string json = await GetAsync($"/api/equipment/ioPorts/{id}",
                                         timeoutMs).ConfigureAwait(false);
            return Deserialize<IoConfig>(json, $"GET /equipment/ioPorts/{id}");
        }

        /// <summary>PUT /api/equipment/ioPorts/{id}.</summary>
        public async Task<IoConfig> SetIoPortAsync(int id, IoConfig port,
                                                   uint timeoutMs = 10000)
        {
            if (port == null) throw new ArgumentNullException(nameof(port));
            string json = await PutAsync($"/api/equipment/ioPorts/{id}",
                                         Serialize(port), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<IoConfig>(json, $"PUT /equipment/ioPorts/{id}");
        }

        /// <summary>GET /api/equipment/ioBasic (pin metadata array).</summary>
        public async Task<List<IoBasic>> GetIoBasicAsync(
            uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/equipment/ioBasic", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<List<IoBasic>>(json, "GET /equipment/ioBasic");
        }

        /// <summary>PUT /api/equipment/ioBasic (bulk update).</summary>
        public async Task<List<IoBasic>> SetIoBasicAsync(
            IEnumerable<IoBasic> ports, uint timeoutMs = 10000)
        {
            if (ports == null) throw new ArgumentNullException(nameof(ports));
            string json = await PutAsync("/api/equipment/ioBasic",
                                         Serialize(ports), timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<List<IoBasic>>(json, "PUT /equipment/ioBasic");
        }

        // ---- REST API client (webhook) servers ----------------------------

        /// <summary>GET /api/equipment/servers/restapiclient/{id}/config.</summary>
        public async Task<RestApiClientConfig> GetRestApiClientConfigAsync(
            int id, uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                $"/api/equipment/servers/restapiclient/{id}/config", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<RestApiClientConfig>(
                json, $"GET /equipment/servers/restapiclient/{id}/config");
        }

        /// <summary>PUT /api/equipment/servers/restapiclient/{id}/config.</summary>
        public async Task<RestApiClientConfig> SetRestApiClientConfigAsync(
            int id, RestApiClientConfig config, uint timeoutMs = 10000)
        {
            if (config == null) throw new ArgumentNullException(nameof(config));
            string json = await PutAsync(
                $"/api/equipment/servers/restapiclient/{id}/config",
                Serialize(config), timeoutMs).ConfigureAwait(false);
            return Deserialize<RestApiClientConfig>(
                json, $"PUT /equipment/servers/restapiclient/{id}/config");
        }

        /// <summary>GET /api/equipment/servers/restapiclient/{id}/status.</summary>
        public async Task<RestApiClientStatus> GetRestApiClientStatusAsync(
            int id, uint timeoutMs = 10000)
        {
            string json = await GetAsync(
                $"/api/equipment/servers/restapiclient/{id}/status", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<RestApiClientStatus>(
                json, $"GET /equipment/servers/restapiclient/{id}/status");
        }

        // ---- Licenses ------------------------------------------------------

        /// <summary>GET /api/system/licenses.</summary>
        public async Task<Licenses> GetLicensesAsync(uint timeoutMs = 10000)
        {
            string json = await GetAsync("/api/system/licenses", timeoutMs)
                .ConfigureAwait(false);
            return Deserialize<Licenses>(json, "GET /system/licenses");
        }

        private delegate NativeErrorCode NoBodyVerb(IntPtr c, string path,
                                                    uint t, out IntPtr o);

        private Task<string> InvokeAsync(string verb, string path,
                                         string body, uint timeoutMs,
                                         NoBodyVerb noBodyFn)
        {
            ThrowIfDisposed();
            if (path == null) throw new ArgumentNullException(nameof(path));
            return Task.Run(() =>
            {
                NativeErrorCode rc;
                IntPtr ptr;
                if (noBodyFn != null)
                {
                    rc = noBodyFn(_handle, path, timeoutMs, out ptr);
                }
                else if (verb == "PUT")
                {
                    rc = NativeMethods.Rest_httpPut(_handle, path,
                                                    body ?? "", timeoutMs,
                                                    out ptr);
                }
                else
                {
                    rc = NativeMethods.Rest_httpPost(_handle, path,
                                                     body ?? "", timeoutMs,
                                                     out ptr);
                }
                if (rc != NativeErrorCode.Ok)
                {
                    if (ptr != IntPtr.Zero) NativeMethods.String_destroy(ptr);
                    ItscamException.ThrowIfFailed(rc, $"{verb} {path}");
                }
                return NativeMethods.TakeString(ptr);
            });
        }

        // ====================================================================
        //  Lifecycle
        // ====================================================================

        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            if (_handle != IntPtr.Zero)
            {
                NativeMethods.Rest_destroy(_handle);
                _handle = IntPtr.Zero;
            }
            GC.SuppressFinalize(this);
        }

        ~ItscamRestClient() { Dispose(); }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(ItscamRestClient));
        }
    }
}
