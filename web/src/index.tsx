import { render } from 'preact';
import { useEffect, useRef, useState } from 'preact/hooks';

import './style.scss';
import createWasmModule from './wasm_module/mcufont_converter';

export function App() {
	let Module: Object;
	const ARRAY_RESULT_SIZE = 2;

	const [resultFontSize, setResultFontSize] = useState<number>(14);
	const [fontFileName, setFontFileName] = useState<string>();
	const [dataFile, setDataFile] = useState<Uint8Array>();
	const [monochromeModeEnabled, setMonochromeModeEnabled] = useState<boolean>(true);
	const [optimizationEnabled, setOptimizationEnabled] = useState<boolean>(true);
	const [optimizationIterations, setOptimizationIterations] = useState<number>(50);
	const canvasPreviewRef = useRef();
	const [canvasWidth, setCanvasWidth] = useState<number>(300);
	const [canvasHeight, setCanvasHeight] = useState<number>(150);
	const [previewText, setPreviewText] = useState<string>('Abc1230');
	const [fontImportProcessing, setFontImportProcessing] = useState<boolean>(true);

	useEffect(() => {
		createWasmModule().then((module) => {
			Module = module;
		});
	});

	function downloadURL(data: string, fileName: string) {
		const a = document.createElement('a');
		a.href = data;
		a.download = fileName;
		document.body.appendChild(a);
		a.style = 'display: none';
		a.click();
		a.remove();
	}

	function downloadBlob(data: Uint8Array, fileName: string, mimeType: string) {
		const blob = new Blob([data], {
			type: mimeType
		});
		const url = window.URL.createObjectURL(blob);
		downloadURL(url, fileName);
		setTimeout(function() {
			return window.URL.revokeObjectURL(url);
		}, 1000);
	}

	async function handleSelectFile(event) {
		const file = event.target.files[0];
		setFontFileName(file.name);
		const fileData = new Uint8Array(await file.arrayBuffer());

		const fileDataPtr = Module._malloc(fileData.length);
		Module.HEAP8.set(fileData, fileDataPtr);
		
		setFontImportProcessing(true);
		
		const resultPtr = Module._import_ttf(fileDataPtr, fileData.length, resultFontSize, monochromeModeEnabled);
		
		Module._free(fileDataPtr);
		const result = new Uint32Array(Module.HEAP32.buffer, resultPtr, ARRAY_RESULT_SIZE);
		Module._free(resultPtr);
		const resultData = new Uint8Array(Module.HEAP8.buffer, result[1], result[0]);
		setDataFile(resultData);

		if (optimizationEnabled) {
			const dataFilePtr = Module._malloc(resultData.length);
			Module.HEAP8.set(resultData, dataFilePtr);
			
			Module._optimize(dataFilePtr, resultData.length, optimizationIterations);
			
			Module._free(dataFilePtr);
		}

		setFontImportProcessing(false);
		renderPreview(resultData, previewText);
	}

	function renderPreview(dataFile: Uint8Array, previewText: string) {
		const text = new TextEncoder().encode(previewText + '\0');
		const textPtr = Module._malloc(text.length);
		Module.HEAP8.set(text, textPtr);

		const dataFilePtr = Module._malloc(dataFile.length);
		Module.HEAP8.set(dataFile, dataFilePtr);

		const resultView = new Uint8Array(Module.HEAP8.buffer, 
			Module._render(dataFilePtr, dataFile.length, canvasWidth, textPtr), canvasWidth * Module._get_result_height());
		
		Module._free(dataFilePtr);
		Module._free(textPtr);
		const result = new Uint8Array(resultView);
		Module._free(resultView);

		const ctx = canvasPreviewRef.current.getContext('2d');
		ctx.clearRect(0, 0, canvasWidth, canvasHeight);

		const WIDTH = canvasWidth;
		const HEIGHT = Module._get_result_height();

		const pixels = new Uint8ClampedArray(WIDTH * HEIGHT * 4);
		for (let y = 0; y < HEIGHT; y++) {
			for (let x = 0; x < WIDTH; x++) {
				const i = (y*WIDTH + x) * 4;
				const result_index = (y*WIDTH + x);

				pixels[i] = result[result_index];
				pixels[i+1] = result[result_index];
				pixels[i+2] = result[result_index];
				pixels[i+3] = 255;
			}
		}

		const imageData = new ImageData(pixels, WIDTH, HEIGHT);
		ctx.putImageData(imageData, 0, 0);
	}

	function handlePreviewText(event) {
		setPreviewText(event.target.value);
		renderPreview(dataFile, event.target.value);
	}

	function handleDownloadFontCode() {
		const dataFilePtr = Module._malloc(dataFile.length);
		Module.HEAP8.set(dataFile, dataFilePtr);
		const fileName = fontFileName.replace(/\.[^/.]+$/, '') + resultFontSize + 'bw';
		const fontName = new TextEncoder().encode(fileName + '\0');
		const fontNamePtr = Module._malloc(fontName.length);
		Module.HEAP8.set(fontName, fontNamePtr);

		const fontCodePtr = Module._export_to_c_code(dataFilePtr, dataFile.length, fontNamePtr);

		Module._free(fontNamePtr);
		Module._free(dataFilePtr);

		const fontCodeArrayResult = new Uint32Array(Module.HEAP32.buffer, fontCodePtr, ARRAY_RESULT_SIZE);
		const codeDataResult = new Uint8Array(Module.HEAP8.buffer, fontCodeArrayResult[1], fontCodeArrayResult[0]);
		Module._free(fontCodePtr);
		Module._free(fontCodeArrayResult[1]);

		downloadBlob(codeDataResult, fileName + '.c', 'application/octet-stream');
	}

	return (
		<>
			<section>
				<div>1. Import TTF font file</div>
				<label>
					Result font size
					<input type="number" value={resultFontSize} onChange={e => setResultFontSize(e.target.value)} />
				</label>
				<div>
					<div>
						<label title="The FT_LOAD_TARGET_MONO and FT_LOAD_MONOCHROME flags will be used">
							<input type="checkbox" checked={monochromeModeEnabled} onChange={e => setMonochromeModeEnabled(e.target.checked)} />
							Monochrome mode
						</label>
					</div>
					<label>
						<input type="checkbox" checked={optimizationEnabled} onChange={e => setOptimizationEnabled(e.target.checked)} />
						Enable font optimization
					</label>
					<div>
						<label>
							Iterations
							<input type="number" value={optimizationIterations} onChange={e => setOptimizationIterations(e.target.value)} disabled={!optimizationEnabled} />
						</label>
					</div>
				</div>
				<input type="file" accept=".ttf" onChange={handleSelectFile} />
			</section>
			<section>
				<div>2. Preview</div>
				<div>
					<input type="text" value={previewText} onKeyUp={handlePreviewText} disabled={fontImportProcessing} />
				</div>
				<canvas ref={canvasPreviewRef} class="preview-canvas" width={canvasWidth} height={canvasHeight}></canvas>
			</section>
			<section>
				<div>3. Download font code</div>
				<button onClick={handleDownloadFontCode} disabled={fontImportProcessing}>Download C file</button>
			</section>
			<footer>
				<div>
					<a href="https://github.com/HexRx/mcufont-converter-web" target="_blank">Github</a>
				</div>
				<div>Know issues and limitations:</div>
				<ul>
					<li>Supports TTF fonts only</li>
					<li>Font optimization doesn't support multi-treading</li>
					<li>Doesn't have ability to filter glyphs</li>
				</ul>
			</footer>
		</>
	);
}

render(<App />, document.getElementById('app'));
