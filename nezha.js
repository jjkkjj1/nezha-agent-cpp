let inited_gpu_info = false;

function handleNodes(servers){
    let nodes = []
    servers?.forEach(server => {
        let arch = server.Host.Arch;
        let new_line_index = arch.indexOf('\n');
        if(new_line_index != "-1"){
            let gpu_base_info = arch.substring(new_line_index+1);
            server.Host.GPU = gpu_base_info.substring(gpu_base_info.indexOf('：')+1)+']';
            server.Host.Arch = arch.substring(0, new_line_index-1);
            console.log(arch, server.Host.GPU);
            if(!inited_gpu_info){
                inited_gpu_info = true;
                let node = document.getElementsByClassName("table table-striped table-condensed table-hover")[0];
                let tr  = node.childNodes[0].childNodes[2];
                let ref_node = tr.children[8];
                let dst_node = document.createElement("th");
                dst_node.className="node-cell hdd center";
                dst_node.innerHTML="GPU";
                tr.insertBefore(dst_node, ref_node);
            }

            let t_id = "t"+server.ID+"_GPU";
            if(!document.getElementById(t_id)){
                let span = document.createElement("span");
                span.className = "node-cell-expand";

                let span2 = document.createElement("span");
                span2.className = "node-cell-expand-label";
                span2.innerHTML = "GPU: ";
                span.id = t_id;
                span.innerHTML = span2.outerHTML + server.Host.GPU;

                let t_parent_node = document.getElementById("rt"+server.ID).children[0];
                let t_ref_node = t_parent_node.children[2];
                t_parent_node.insertBefore(span, t_ref_node);
            }

        }else{
            server.Host.GPU = 0;
        }

        let processCount = server.State.ProcessCount;
        if(processCount > 99999){
            server.State.ProcessCount = processCount%100000;
            server.State.GPU = ((processCount+0.0)/100000)/100;
        }else{
            server.State.GPU = 0;
        }

        if(inited_gpu_info){
            let gpu_percent_node_id = "r"+server.ID+"_GPU";
            let gpu_percent_node = document.getElementById(gpu_percent_node_id);
            if(!gpu_percent_node){
                gpu_percent_node = document.createElement("td");
                gpu_percent_node.className="node-cell cpu";
                let div1 = document.createElement("div");
                div1.className = server.live?"progress progress-online":"progress progress-offline";
                let div2 = document.createElement("div");
                div2.className="progress-bar progress-bar-success";
                let percent_str = server.State.GPU.toFixed(2)+"%";
                div2.style = "width:" + percent_str+";";
                let small = document.createElement("small");
                small.innerHTML = percent_str;
                div2.appendChild(small);
                div1.appendChild(div2);
                gpu_percent_node.appendChild(div1);
                gpu_percent_node.id = gpu_percent_node_id;

                let parent_node = document.getElementById("r"+server.ID);
                let ref_node = parent_node.children[8];
                parent_node.insertBefore(gpu_percent_node, ref_node);
                if(server.Host.GPU){
                    gpu_percent_node.style.visibility="visible";
                }else{
                    gpu_percent_node.style.visibility="hidden";
                }
            }else if(server.Host.GPU){
                let percent_str = server.State.GPU.toFixed(2)+"%";
                gpu_percent_node.children[0].children[0].style = "width:" + percent_str+";";
                gpu_percent_node.children[0].children[0].children[0].innerHTML = percent_str;
                gpu_percent_node.children[0].className = server.live?"progress progress-online":"progress progress-offline";
                if(server.Host.GPU){
                    gpu_percent_node.style.visibility="visible";
                }else{
                    gpu_percent_node.style.visibility="hidden";
                }
            }

            let txt_node_id = "t"+server.ID+"_GPU";
            let txt_node = document.getElementById(txt_node_id);
            if(txt_node){
                let span2 = document.createElement("span");
                span2.className = "node-cell-expand-label";
                span2.innerHTML = "GPU: ";
                txt_node.innerHTML = span2.outerHTML + server.Host.GPU;
            }
        }

        let platform = server.Host.Platform;
        if (this.isWindowsPlatform(platform)) {
            platform = "windows"
        }else if (platform === "immortalwrt") {
            platform = "openwrt"
        }
        let node = {
            ID: server.ID,
            name: server.Name,
            os: platform,
            location: server.Host.CountryCode,
            uptime: this.secondToDate(server.State.Uptime),
            load: this.toFixed2(server.State.Load1),
            network: this.getNetworkSpeed(server.State.NetInSpeed, server.State.NetOutSpeed),
            traffic: this.formatByteSize(server.State.NetInTransfer) + ' | ' + this.formatByteSize(server.State.NetOutTransfer),
            cpu: this.formatPercents(this.toFixed2(server.State.CPU)),
            memory: this.formatPercent(server.live, server.State.MemUsed, server.Host.MemTotal),
            hdd: this.formatPercent(server.live, server.State.DiskUsed, server.Host.DiskTotal),
            online: server.live,
            state: server.State,
            host: server.Host,
            lastActive: server.LastActive,
            Tag: server.Tag,
        }
        nodes.push(node)
    })
    return nodes;
}

document.addEventListener('DOMContentLoaded', function() {
    document.querySelector('#app').__vue__.handleNodes = handleNodes;
});