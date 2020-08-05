;(function(){
  
  function setup() {
    const canvas = document.getElementById('blocks-canvas');
    const gl = canvas.getContext('webgl');

    if (gl === null) {
      alert("Unable to initialize WebGL. Your browser or machine may not support it.");
      return;
    }
    
    const programInfo = initShaders(gl);
    const vertexBuffer = gl.createBuffer();
    
    const renderInfo = runBlocks();
    
    draw(gl, programInfo, vertexBuffer, renderInfo);
    
  }
  
  function initShaders(gl) {
    const vertexShaderSource = `
      attribute vec2 position;
      attribute vec4 color;
      
      uniform mat4 projection;
      
      varying highp vec4 vertColor;
      
      void main() {
        vec4 pos = vec4(position.xy, 0, 1.0);
        vertColor = color;
        gl_Position = projection * pos;
      }
    `;
    
    const fragShaderSource = `
      varying highp vec4 vertColor;
      
      void main() {
        gl_FragColor = vertColor;
      }
    `;
    
    const vertShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertShader, vertexShaderSource);
    gl.compileShader(vertShader);
    if (!gl.getShaderParameter(vertShader, gl.COMPILE_STATUS)) {
      alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(vertShader));
      gl.deleteShader(vertShader);
      return;
    }
    
    const fragShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragShader, fragShaderSource);
    gl.compileShader(fragShader);
    if (!gl.getShaderParameter(fragShader, gl.COMPILE_STATUS)) {
      alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(fragShader));
      gl.deleteShader(fragShader);
      return;
    }
    
    const shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertShader);
    gl.attachShader(shaderProgram, fragShader);
    gl.linkProgram(shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
      alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
      return;
    }
    
    const programInfo = {
      program: shaderProgram,
      attribs: {
        position: gl.getAttribLocation(shaderProgram, 'position'),
        color: gl.getAttribLocation(shaderProgram, 'color'),
      },
      uniforms: {
        projection: gl.getUniformLocation(shaderProgram, 'projection'),
      },
    };
    
    return programInfo;
  }
  
  function runBlocks() {
    
    var blocksMem = Module._malloc(1024 * 1024 * 4);
    var result = Module._malloc(1024);
    
    Module._InitBlocks(blocksMem, 1024 * 1024 * 4);
    
    var inputBuf = Module._malloc(8 * 4);
    Module.setValue(inputBuf + (4 * 3), 640.0, 'float');
    Module.setValue(inputBuf + (4 * 4), 480.0, 'float');
    
    Module._RunBlocks(result, blocksMem, inputBuf);
    
    var vertexData = Module.getValue(result, 'i32');
    var vertexDataSize = Module.getValue(result + 4, 'i32');
    
    var drawCallCount = Module.getValue(result + 1160, 'i32');
    
    var drawCalls = [];
    var drawCallBase = result + 8;
    var drawCallSize = 16 * 18 * 4;
    for (var i = 0; i < 16; ++i) {
      var drawCall = {transform: [], vertexCount: 0, vertexOffset: 0};
      for (var j = 0; j < 16; ++j) {
        drawCall.transform.push(Module.getValue(drawCallBase + (drawCallSize * i) + (j * 4), 'float'));
      }
      drawCall.vertexCount = Module.getValue(drawCallBase + (drawCallSize * i) + (16 * 4), 'i32');
      drawCall.vertexOffset = Module.getValue(drawCallBase + (drawCallSize * i) + (17 * 4), 'i32');
      drawCalls.push(drawCall);
    }
    
    return {
      vertexData: vertexData,
      vertexDataSize: vertexDataSize,
      drawCalls: drawCalls,
      drawCallCount: drawCallCount
    };
    
  }
  
  function draw(gl, programInfo, vertexBuffer, renderInfo) {
    
    var vertexData = renderInfo.vertexData;
    var vertexDataSize = renderInfo.vertexDataSize;
    
    // copy vertex data
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    // const positions = [
    //   -0.5,  0.5,
    //    0.5,  0.5,
    //   -0.5, -0.5,
    //    0.5, -0.5,
    // ];
    const vertData = Module.HEAPU8.subarray(vertexData, vertexData + vertexDataSize)
    gl.bufferData(gl.ARRAY_BUFFER, vertData, gl.STATIC_DRAW);
    
    // Position
    {
      const numComponents = 2;  // pull out 2 values per iteration
      const type = gl.FLOAT;    // the data in the buffer is 32bit floats
      const normalize = false;  // don't normalize
      const stride = 12 * 4;         // how many bytes to get from one set of values to the next
                                     // 0 = use type and numComponents above
      const offset = 0;         // how many bytes inside the buffer to start from
      gl.vertexAttribPointer(
          programInfo.attribs.position,
          numComponents,
          type,
          normalize,
          stride,
          offset);
      gl.enableVertexAttribArray(
          programInfo.attribs.position);
    }
    
    // Color
    {
      const numComponents = 4;  // pull out 2 values per iteration
      const type = gl.FLOAT;    // the data in the buffer is 32bit floats
      const normalize = false;  // don't normalize
      const stride = 12 * 4;         // how many bytes to get from one set of values to the next
                                     // 0 = use type and numComponents above
      const offset = 4 * 4;         // how many bytes inside the buffer to start from
      gl.vertexAttribPointer(
          programInfo.attribs.color,
          numComponents,
          type,
          normalize,
          stride,
          offset);
      gl.enableVertexAttribArray(
          programInfo.attribs.color);
    }
    
    
    gl.clearColor(0x33 / 255.0, 0x47 / 255.0, 0x71 / 255.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);
    
    var drawCall = renderInfo.drawCalls[0];
    
    const projection = drawCall.transform;
    
    gl.useProgram(programInfo.program);
    gl.uniformMatrix4fv(
      programInfo.uniforms.projection,
      false,
      projection);
    
    
    {
      const offset = drawCall.vertexOffset;
      const vertexCount = drawCall.vertexCount;
      gl.drawArrays(gl.TRIANGLES, offset, vertexCount);
    }
    
  }
  
  Module.onRuntimeInitialized = setup;
  
})();