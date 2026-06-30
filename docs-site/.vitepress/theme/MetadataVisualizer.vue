<script setup lang="ts">
import { useRoute } from "vitepress";
import { computed, onBeforeUnmount, ref } from "vue";

type MetadataRow = {
  index: number;
  key: string;
  value: string;
};

type BoundingBox = {
  id: string;
  kind: "plate" | "vehicle";
  label: string;
  x: number;
  y: number;
  width: number;
  height: number;
};

type Exposure = {
  index: number;
  url: string;
  rawComment: string;
  rows: MetadataRow[];
  boxes: BoundingBox[];
  width: number;
  height: number;
};

const JPEG_START_TAG = 0xd8;
const JPEG_SECTION_TAG = 0xff;
const JPEG_COMMENT_TAG = 0xfe;
const JPEG_END_TAG = 0xd9;
const COMMENT_LENGTH_BYTES = 2;
const COMMENT_HEADER_BYTES = 4;
const PLATE_EXPANSION_PX = 5;
const PLATE_LABEL_HEIGHT_PX = 24;

const fileName = ref("");
const error = ref("");
const search = ref("");
const sortKey = ref<"index" | "key" | "value">("index");
const sortDirection = ref<"asc" | "desc">("asc");
const activeBoxId = ref("");
const exposures = ref<Exposure[]>([]);
const activeExposureIndex = ref(0);
const route = useRoute();

const labels = computed(() => {
  const isEnglish = route.path.includes(".en-US");
  return isEnglish
    ? {
        uploadTitle: "Upload a Pumatronix JPEG or multipart capture",
        uploadCopy:
          "The file stays in your browser. The page extracts each JPEG exposure, reads the COM marker, and draws plate and vehicle boxes when the tags include coordinates.",
        noExposureComment: "No JPEG COM marker was found in this exposure.",
        emptyExposureComment:
          "The COM marker was found, but it has no key=value tags.",
        invalidCommentLength: "Invalid JPEG COM marker length.",
        noJpegImage: "No JPEG image was found in this file.",
        noCommentInAnyExposure:
          "No JPEG COM marker was found in any exposure.",
        couldNotReadFile: "Could not read file.",
        plate: "Plate",
        vehicle: "Vehicle",
        vehicleTypes: {
          0: "Unknown",
          1: "Car",
          2: "Motorcycle",
          3: "Truck",
          4: "Bus",
          5: "Pickup",
          6: "SUV",
          7: "Van",
          8: "Tow",
        } as Record<number, string>,
        exposure: "Exposure",
        previous: "Previous",
        next: "Next",
        imageAlt: "Uploaded JPEG exposure preview",
        searchMetadata: "Search metadata",
        searchPlaceholder: "Filter by tag or value",
        tags: "tags",
        tagColumn: "Tag",
        valueColumn: "Value",
        emptyRows: "No metadata tags to show.",
        rawComment: "Raw COM marker",
      }
    : {
        uploadTitle: "Envie um JPEG ou captura multipart Pumatronix",
        uploadCopy:
          "O arquivo fica no seu navegador. A página extrai cada exposição JPEG, lê o COM marker e desenha caixas de placas e veículos quando as tags incluem coordenadas.",
        noExposureComment: "Nenhum COM marker JPEG foi encontrado nesta exposição.",
        emptyExposureComment:
          "O COM marker foi encontrado, mas não contém tags chave=valor.",
        invalidCommentLength: "Tamanho inválido do COM marker JPEG.",
        noJpegImage: "Nenhuma imagem JPEG foi encontrada neste arquivo.",
        noCommentInAnyExposure:
          "Nenhum COM marker JPEG foi encontrado nas exposições.",
        couldNotReadFile: "Não foi possível ler o arquivo.",
        plate: "Placa",
        vehicle: "Veículo",
        vehicleTypes: {
          0: "Desconhecido",
          1: "Carro",
          2: "Motocicleta",
          3: "Caminhão",
          4: "Ônibus",
          5: "Pickup",
          6: "SUV",
          7: "Van",
          8: "Guincho",
        } as Record<number, string>,
        exposure: "Exposição",
        previous: "Anterior",
        next: "Próxima",
        imageAlt: "Prévia da exposição JPEG enviada",
        searchMetadata: "Pesquisar metadados",
        searchPlaceholder: "Filtrar por tag ou valor",
        tags: "tags",
        tagColumn: "Tag",
        valueColumn: "Valor",
        emptyRows: "Nenhuma tag de metadados para exibir.",
        rawComment: "COM marker bruto",
      };
});

const activeExposure = computed(() => exposures.value[activeExposureIndex.value]);
const activeRows = computed(() => activeExposure.value?.rows ?? []);
const activeBoxes = computed(() => activeExposure.value?.boxes ?? []);
const drawableBoxes = computed(() =>
  activeExposure.value?.width && activeExposure.value?.height
    ? activeBoxes.value
    : [],
);
const rawComment = computed(() => activeExposure.value?.rawComment ?? "");
const activeNotice = computed(() => {
  if (error.value) return error.value;
  if (!activeExposure.value) return "";
  if (!activeExposure.value.rawComment) {
    return labels.value.noExposureComment;
  }
  if (activeExposure.value.rows.length === 0) {
    return labels.value.emptyExposureComment;
  }
  return "";
});

const filteredRows = computed(() => {
  const needle = search.value.trim().toLowerCase();
  const rows = activeRows.value;
  const filtered = needle
    ? rows.filter((row) =>
        `${row.key} ${row.value}`.toLowerCase().includes(needle),
      )
    : rows;

  return [...filtered].sort((left, right) => {
    const direction = sortDirection.value === "asc" ? 1 : -1;
    if (sortKey.value === "index") {
      return (left.index - right.index) * direction;
    }
    return left[sortKey.value].localeCompare(right[sortKey.value]) * direction;
  });
});

function extractJpegComment(bytes: Uint8Array): string {
  for (let index = bytes.length - 2; index > 0; index -= 1) {
    if (
      bytes[index] === JPEG_SECTION_TAG &&
      bytes[index + 1] === JPEG_COMMENT_TAG
    ) {
      if (index + COMMENT_HEADER_BYTES > bytes.length) {
        throw new Error(labels.value.invalidCommentLength);
      }
      const commentLength =
        bytes[index + 2] * 256 + bytes[index + 3] - COMMENT_LENGTH_BYTES;
      const start = index + COMMENT_HEADER_BYTES;
      const end = start + commentLength;
      if (!Number.isFinite(commentLength) || commentLength < 0 || end > bytes.length) {
        throw new Error(labels.value.invalidCommentLength);
      }

      let comment = "";
      const chunkSize = 8192;
      for (let offset = start; offset < end; offset += chunkSize) {
        comment += String.fromCharCode(
          ...bytes.slice(offset, Math.min(offset + chunkSize, end)),
        );
      }
      return comment;
    }
  }

  return "";
}

function parseTags(rows: MetadataRow[]): Record<string, string> {
  const tags: Record<string, string> = {};
  for (const row of rows) {
    tags[row.key] = row.value;
  }
  return tags;
}

function parseRows(comment: string): MetadataRow[] {
  return comment
    .split(";")
    .map((field, index) => {
      const equalsAt = field.indexOf("=");
      if (equalsAt <= 0) return null;
      return {
        index: index + 1,
        key: field.slice(0, equalsAt),
        value: field.slice(equalsAt + 1),
      };
    })
    .filter((row): row is MetadataRow => row !== null);
}

function parseNumber(value: unknown): number {
  const parsed = Number.parseInt(String(value ?? "").trim(), 10);
  return Number.isFinite(parsed) ? parsed : 0;
}

function parseBracketList(raw: string | undefined): unknown[][] {
  if (!raw) return [];
  try {
    const parsed = JSON.parse(`[${raw}]`);
    return Array.isArray(parsed) ? parsed.filter(Array.isArray) : [];
  } catch {
    return [];
  }
}

function buildBoxes(tags: Record<string, string>): BoundingBox[] {
  const nextBoxes: BoundingBox[] = [];
  const plateTexts = (tags.Placa || "").split("_");
  const plateCoords = (tags.CoordPlaca || "").split("_");

  for (let index = 0; index < plateCoords.length; index += 1) {
    const [position, size] = plateCoords[index].split(",");
    const [x, y] = (position || "").split("x").map(parseNumber);
    const [width, height] = (size || "").split("x").map(parseNumber);
    if (!width || !height) continue;

    nextBoxes.push({
      id: `plate-${index}`,
      kind: "plate",
      label: `${labels.value.plate}${plateTexts[index] ? ` - ${plateTexts[index]}` : ""}`,
      x: Math.max(0, x - PLATE_EXPANSION_PX),
      y: Math.max(0, y - PLATE_EXPANSION_PX),
      width: width + PLATE_EXPANSION_PX * 2,
      height: height + PLATE_EXPANSION_PX * 2,
    });
  }

  const classifierEntries = parseBracketList(tags.ClassifierList);
  const bmcEntries = parseBracketList(tags.BMCList);
  for (let index = 0; index < classifierEntries.length; index += 1) {
    const [type, probability, x, y, width, height] = classifierEntries[index];
    const parsedWidth = parseNumber(width);
    const parsedHeight = parseNumber(height);
    if (!parsedWidth || !parsedHeight) continue;

    const bmc = bmcEntries[index] || [];
    const vehicleType = labels.value.vehicleTypes[parseNumber(type)] || labels.value.vehicle;
    const details = [bmc[0], bmc[2], bmc[4]].filter(Boolean).join(" ");

    nextBoxes.push({
      id: `vehicle-${index}`,
      kind: "vehicle",
      label: `${labels.value.vehicle} - ${vehicleType} ${parseNumber(probability)}%${
        details ? ` - ${details}` : ""
      }`,
      x: parseNumber(x),
      y: parseNumber(y),
      width: parsedWidth,
      height: parsedHeight,
    });
  }

  return nextBoxes;
}

function boxStyle(box: BoundingBox) {
  const width = activeExposure.value?.width;
  const height = activeExposure.value?.height;
  if (!width || !height) return { display: "none" };

  return {
    left: `${(box.x / width) * 100}%`,
    top: `${(box.y / height) * 100}%`,
    width: `${(box.width / width) * 100}%`,
    height: `${(box.height / height) * 100}%`,
  };
}

function boxLabelClass(box: BoundingBox) {
  if (box.kind !== "plate") return "metadata-visualizer__box-label--inside";

  const imageHeight = activeExposure.value?.height || 0;
  const roomAbove = box.y;
  const roomBelow = Math.max(0, imageHeight - (box.y + box.height));
  return roomAbove >= PLATE_LABEL_HEIGHT_PX || roomAbove >= roomBelow
    ? "metadata-visualizer__box-label--above"
    : "metadata-visualizer__box-label--below";
}

function findMarker(bytes: Uint8Array, marker: number, start: number): number {
  for (let index = Math.max(0, start); index < bytes.length - 1; index += 1) {
    if (bytes[index] === JPEG_SECTION_TAG && bytes[index + 1] === marker) {
      return index;
    }
  }
  return -1;
}

function extractJpegImages(bytes: Uint8Array): Uint8Array[] {
  const images: Uint8Array[] = [];
  let cursor = 0;

  while (cursor < bytes.length - 1) {
    const start = findMarker(bytes, JPEG_START_TAG, cursor);
    if (start < 0) break;

    const end = findMarker(bytes, JPEG_END_TAG, start + 2);
    if (end < 0) break;

    images.push(bytes.slice(start, end + 2));
    cursor = end + 2;
  }

  return images;
}

function imageUrlFromBytes(bytes: Uint8Array): string {
  const blob = new Blob(
    [bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength)],
    { type: "image/jpeg" },
  );
  return URL.createObjectURL(blob);
}

function revokeExposureUrls() {
  for (const exposure of exposures.value) {
    URL.revokeObjectURL(exposure.url);
  }
}

function selectExposure(index: number) {
  if (index < 0 || index >= exposures.value.length) return;
  activeExposureIndex.value = index;
  activeBoxId.value = "";
}

function stepExposure(delta: number) {
  const count = exposures.value.length;
  if (count <= 1) return;
  selectExposure((activeExposureIndex.value + delta + count) % count);
}

function sortBy(key: "index" | "key" | "value") {
  if (sortKey.value === key) {
    sortDirection.value = sortDirection.value === "asc" ? "desc" : "asc";
    return;
  }
  sortKey.value = key;
  sortDirection.value = "asc";
}

function resetPreview() {
  revokeExposureUrls();
  exposures.value = [];
  error.value = "";
  activeExposureIndex.value = 0;
  activeBoxId.value = "";
}

async function loadFile(event: Event) {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  if (!file) return;

  let nextExposures: Exposure[] = [];

  resetPreview();
  fileName.value = file.name;

  try {
    const bytes = new Uint8Array(await file.arrayBuffer());
    const images = extractJpegImages(bytes);
    if (images.length === 0) {
      error.value = labels.value.noJpegImage;
      return;
    }

    let firstParseError = "";
    nextExposures = images.map((imageBytes, index) => {
      const exposure: Exposure = {
        index,
        url: imageUrlFromBytes(imageBytes),
        rawComment: "",
        rows: [],
        boxes: [],
        width: 0,
        height: 0,
      };

      try {
        exposure.rawComment = extractJpegComment(imageBytes);
        exposure.rows = parseRows(exposure.rawComment);
        exposure.boxes = buildBoxes(parseTags(exposure.rows));
      } catch (caught) {
        if (!firstParseError) {
          firstParseError =
            caught instanceof Error ? caught.message : labels.value.couldNotReadFile;
        }
      }

      return exposure;
    });
    exposures.value = nextExposures;

    if (firstParseError) {
      error.value = firstParseError;
    } else if (!exposures.value.some((exposure) => exposure.rawComment)) {
      error.value = labels.value.noCommentInAnyExposure;
    }
  } catch (caught) {
    for (const exposure of nextExposures) {
      URL.revokeObjectURL(exposure.url);
    }
    error.value = caught instanceof Error ? caught.message : labels.value.couldNotReadFile;
  } finally {
    input.value = "";
  }
}

function onImageLoad(event: Event) {
  const image = event.target as HTMLImageElement;
  if (!activeExposure.value) return;
  activeExposure.value.width = image.naturalWidth;
  activeExposure.value.height = image.naturalHeight;
}

onBeforeUnmount(() => {
  revokeExposureUrls();
});
</script>

<template>
  <section class="metadata-visualizer">
    <label class="metadata-visualizer__drop">
      <input
        type="file"
        accept="image/jpeg,.jpg,.jpeg,.mjpg,.mjpeg,.multipart"
        @change="loadFile"
      />
      <span class="metadata-visualizer__drop-title">
        {{ labels.uploadTitle }}
      </span>
      <span class="metadata-visualizer__drop-copy">
        {{ labels.uploadCopy }}
      </span>
    </label>

    <p v-if="activeNotice" class="metadata-visualizer__notice">
      {{ activeNotice }}
    </p>

    <div v-if="activeExposure" class="metadata-visualizer__workspace">
      <div class="metadata-visualizer__preview-panel">
        <div class="metadata-visualizer__preview-heading">
          <strong>{{ fileName }}</strong>
          <span
            v-if="activeExposure.width && activeExposure.height"
            class="metadata-visualizer__preview-meta"
          >
            {{ labels.exposure }} {{ activeExposure.index + 1 }} / {{ exposures.length }} -
            {{ activeExposure.width }} x {{ activeExposure.height }} px
          </span>
        </div>

        <div v-if="exposures.length > 1" class="metadata-visualizer__carousel">
          <button type="button" @click="stepExposure(-1)">
            {{ labels.previous }}
          </button>
          <div class="metadata-visualizer__carousel-track">
            <button
              v-for="exposure in exposures"
              :key="exposure.url"
              :class="{
                'metadata-visualizer__carousel-dot--active':
                  exposure.index === activeExposureIndex,
              }"
              type="button"
              @click="selectExposure(exposure.index)"
            >
              {{ exposure.index + 1 }}
            </button>
          </div>
          <button type="button" @click="stepExposure(1)">
            {{ labels.next }}
          </button>
        </div>

        <div class="metadata-visualizer__image-wrap">
          <img
            :key="activeExposure.url"
            :src="activeExposure.url"
            :alt="labels.imageAlt"
            @load="onImageLoad"
          />
          <button
            v-for="box in drawableBoxes"
            :key="box.id"
            class="metadata-visualizer__box"
            :class="[
              `metadata-visualizer__box--${box.kind}`,
              { 'metadata-visualizer__box--active': activeBoxId === box.id },
            ]"
            :style="boxStyle(box)"
            type="button"
            @focus="activeBoxId = box.id"
            @blur="activeBoxId = ''"
            @mouseenter="activeBoxId = box.id"
            @mouseleave="activeBoxId = ''"
          >
            <span :class="boxLabelClass(box)">{{ box.label }}</span>
          </button>
        </div>

        <div v-if="drawableBoxes.length" class="metadata-visualizer__legend">
          <span><i class="metadata-visualizer__swatch metadata-visualizer__swatch--plate" /> {{ labels.plate }}</span>
          <span><i class="metadata-visualizer__swatch metadata-visualizer__swatch--vehicle" /> {{ labels.vehicle }}</span>
        </div>
      </div>

      <div class="metadata-visualizer__table-panel">
        <div class="metadata-visualizer__table-toolbar">
          <label>
            <span>{{ labels.searchMetadata }}</span>
            <input v-model="search" type="search" :placeholder="labels.searchPlaceholder" />
          </label>
          <span class="metadata-visualizer__tag-count">
            {{ filteredRows.length }} / {{ activeRows.length }} {{ labels.tags }}
          </span>
        </div>

        <div class="metadata-visualizer__table-scroll">
          <table>
            <thead>
              <tr>
                <th><button type="button" @click="sortBy('index')">#</button></th>
                <th><button type="button" @click="sortBy('key')">{{ labels.tagColumn }}</button></th>
                <th><button type="button" @click="sortBy('value')">{{ labels.valueColumn }}</button></th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="row in filteredRows" :key="`${row.index}-${row.key}`">
                <td>{{ row.index }}</td>
                <td>{{ row.key }}</td>
                <td>{{ row.value }}</td>
              </tr>
              <tr v-if="activeRows.length === 0">
                <td colspan="3">{{ labels.emptyRows }}</td>
              </tr>
            </tbody>
          </table>
        </div>

        <details v-if="rawComment" class="metadata-visualizer__raw">
          <summary>{{ labels.rawComment }}</summary>
          <pre>{{ rawComment }}</pre>
        </details>
      </div>
    </div>
  </section>
</template>

<style scoped>
.metadata-visualizer {
  display: grid;
  gap: 1rem;
}

.metadata-visualizer__drop {
  display: grid;
  gap: 0.35rem;
  padding: 1.25rem;
  border: 1px dashed var(--vp-c-brand-2);
  border-radius: 8px;
  background:
    linear-gradient(135deg, rgba(40, 132, 108, 0.1), transparent 45%),
    var(--vp-c-bg-soft);
  cursor: pointer;
}

.metadata-visualizer__drop input {
  width: 100%;
}

.metadata-visualizer__drop-title {
  color: var(--vp-c-text-1);
  font-size: 1.1rem;
  font-weight: 700;
}

.metadata-visualizer__drop-copy,
.metadata-visualizer__preview-heading span,
.metadata-visualizer__table-toolbar span {
  color: var(--vp-c-text-2);
  font-size: 0.92rem;
}

.metadata-visualizer__notice {
  margin: 0;
  padding: 0.75rem 0.9rem;
  border: 1px solid var(--vp-c-warning-2);
  border-radius: 8px;
  background: var(--vp-c-warning-soft);
  color: var(--vp-c-text-1);
}

.metadata-visualizer__workspace {
  display: grid;
  grid-template-columns: minmax(0, 1.2fr) minmax(20rem, 0.8fr);
  gap: 1rem;
  align-items: start;
}

.metadata-visualizer__preview-panel,
.metadata-visualizer__table-panel {
  min-width: 0;
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  background: var(--vp-c-bg);
}

.metadata-visualizer__preview-heading {
  display: grid;
  gap: 0.75rem;
  align-items: start;
  padding: 0.85rem 1rem;
  border-bottom: 1px solid var(--vp-c-divider);
}

.metadata-visualizer__preview-heading strong,
.metadata-visualizer__preview-meta {
  min-width: 0;
  overflow-wrap: anywhere;
}

.metadata-visualizer__table-toolbar {
  display: grid;
  grid-template-columns: minmax(0, 1fr) max-content;
  gap: 0.75rem;
  align-items: end;
  padding: 0.85rem 1rem;
  border-bottom: 1px solid var(--vp-c-divider);
}

.metadata-visualizer__carousel {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  justify-content: space-between;
  padding: 0.75rem 1rem;
  border-bottom: 1px solid var(--vp-c-divider);
}

.metadata-visualizer__carousel button {
  min-width: 2.25rem;
  padding: 0.4rem 0.65rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  font: inherit;
  font-size: 0.9rem;
  cursor: pointer;
}

.metadata-visualizer__carousel-track {
  display: flex;
  gap: 0.35rem;
  overflow-x: auto;
  padding: 0.1rem;
}

.metadata-visualizer__carousel .metadata-visualizer__carousel-dot--active {
  border-color: var(--vp-c-brand-2);
  background: var(--vp-c-brand-soft);
  color: var(--vp-c-brand-1);
  font-weight: 700;
}

.metadata-visualizer__image-wrap {
  position: relative;
  overflow: hidden;
  background:
    linear-gradient(45deg, var(--vp-c-bg-soft) 25%, transparent 25%),
    linear-gradient(-45deg, var(--vp-c-bg-soft) 25%, transparent 25%),
    linear-gradient(45deg, transparent 75%, var(--vp-c-bg-soft) 75%),
    linear-gradient(-45deg, transparent 75%, var(--vp-c-bg-soft) 75%);
  background-position: 0 0, 0 8px, 8px -8px, -8px 0;
  background-size: 16px 16px;
}

.metadata-visualizer__image-wrap img {
  display: block;
  width: 100%;
  height: auto;
}

.metadata-visualizer__box {
  position: absolute;
  padding: 0;
  border: 2px solid;
  background: transparent;
  cursor: default;
}

.metadata-visualizer__box span {
  position: absolute;
  left: 0;
  display: block;
  max-width: min(22rem, max(100%, 12rem));
  overflow: hidden;
  padding: 0.15rem 0.4rem;
  border-radius: 4px;
  color: white;
  font-size: 0.78rem;
  font-weight: 700;
  line-height: 1.25;
  opacity: 0.94;
  pointer-events: none;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.metadata-visualizer__box:hover span,
.metadata-visualizer__box--active span {
  opacity: 1;
}

.metadata-visualizer__box--plate {
  border-color: #e5484d;
  box-shadow: inset 0 0 0 1px rgba(229, 72, 77, 0.2);
}

.metadata-visualizer__box--plate span {
  background: #b4232a;
}

.metadata-visualizer__box-label--above {
  bottom: calc(100% + 2px);
}

.metadata-visualizer__box-label--below {
  top: calc(100% + 2px);
}

.metadata-visualizer__box-label--inside {
  top: 0;
  max-width: 100%;
  max-height: 100%;
  border-radius: 0 0 4px 0;
  white-space: normal;
}

.metadata-visualizer__box--vehicle {
  border-color: #23826f;
  box-shadow: inset 0 0 0 1px rgba(35, 130, 111, 0.2);
}

.metadata-visualizer__box--vehicle span {
  background: #176a5a;
}

.metadata-visualizer__legend {
  display: flex;
  gap: 1rem;
  flex-wrap: wrap;
  padding: 0.75rem 1rem;
  border-top: 1px solid var(--vp-c-divider);
  color: var(--vp-c-text-2);
  font-size: 0.9rem;
}

.metadata-visualizer__legend span {
  display: inline-flex;
  gap: 0.35rem;
  align-items: center;
}

.metadata-visualizer__swatch {
  width: 0.8rem;
  height: 0.8rem;
  border: 2px solid;
}

.metadata-visualizer__swatch--plate {
  border-color: #e5484d;
}

.metadata-visualizer__swatch--vehicle {
  border-color: #23826f;
}

.metadata-visualizer__table-toolbar label {
  display: grid;
  gap: 0.25rem;
  min-width: 0;
  color: var(--vp-c-text-2);
  font-size: 0.84rem;
}

.metadata-visualizer__tag-count {
  justify-self: end;
  max-width: 100%;
  padding: 0.45rem 0.6rem;
  border-radius: 6px;
  background: var(--vp-c-bg-soft);
  text-align: right;
  white-space: nowrap;
}

.metadata-visualizer__table-toolbar input {
  width: 100%;
  padding: 0.45rem 0.6rem;
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  background: var(--vp-c-bg);
  color: var(--vp-c-text-1);
}

.metadata-visualizer__table-scroll {
  overflow: auto;
  max-height: 34rem;
}

.metadata-visualizer table {
  width: 100%;
  border-collapse: collapse;
  font-size: 0.9rem;
}

.metadata-visualizer th,
.metadata-visualizer td {
  padding: 0.55rem 0.7rem;
  border-bottom: 1px solid var(--vp-c-divider);
  text-align: left;
  vertical-align: top;
}

.metadata-visualizer th:first-child,
.metadata-visualizer td:first-child {
  width: 3.4rem;
  color: var(--vp-c-text-3);
}

.metadata-visualizer td:last-child {
  overflow-wrap: anywhere;
}

.metadata-visualizer th button {
  padding: 0;
  border: 0;
  background: transparent;
  color: var(--vp-c-text-1);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
}

.metadata-visualizer__raw {
  padding: 0.8rem 1rem 1rem;
}

.metadata-visualizer__raw pre {
  overflow: auto;
  margin: 0.75rem 0 0;
  white-space: pre-wrap;
}

@media (max-width: 900px) {
  .metadata-visualizer__workspace {
    grid-template-columns: 1fr;
  }

  .metadata-visualizer__preview-heading,
  .metadata-visualizer__table-toolbar {
    align-items: flex-start;
  }

  .metadata-visualizer__preview-heading {
    flex-direction: column;
  }

  .metadata-visualizer__table-toolbar {
    grid-template-columns: 1fr;
  }

  .metadata-visualizer__tag-count {
    justify-self: start;
    text-align: left;
  }
}
</style>