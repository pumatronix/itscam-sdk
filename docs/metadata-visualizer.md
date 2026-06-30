---
title: Visualizador de metadados JPEG
description: Faça upload de um JPEG Pumatronix para inspecionar o COM marker e visualizar bounding boxes de placas e veículos.
aside: false
outline: false
---

# Visualizador de metadados JPEG

[Português (Brasil)](metadata-visualizer.md) | [English (US)](metadata-visualizer.en-US.md)

Use esta página para inspecionar os metadados embarcados em imagens JPEG geradas por câmeras ITSCAM. O arquivo é processado localmente no navegador: o visualizador extrai cada exposição JPEG de arquivos simples ou multipart, mostra os pares `chave=valor` em uma tabela pesquisável/ordenável e desenha bounding boxes quando encontra tags como `CoordPlaca` e `ClassifierList`.

<MetadataVisualizer />

Veja também [Metadados JPEG (COM marker)](jpeg-metadata.md) para detalhes sobre o formato e os helpers disponíveis no SDK.