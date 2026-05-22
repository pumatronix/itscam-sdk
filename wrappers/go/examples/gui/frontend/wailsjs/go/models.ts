export namespace main {
	
	export class ConnectionStatus {
	    connected: boolean;
	    state: string;
	    message: string;
	
	    static createFrom(source: any = {}) {
	        return new ConnectionStatus(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.connected = source["connected"];
	        this.state = source["state"];
	        this.message = source["message"];
	    }
	}
	export class DeviceInfo {
	    activeProfile: number;
	    address: string;
	    port: number;
	
	    static createFrom(source: any = {}) {
	        return new DeviceInfo(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.activeProfile = source["activeProfile"];
	        this.address = source["address"];
	        this.port = source["port"];
	    }
	}
	export class SaveResult {
	    success: boolean;
	    filePath: string;
	    error?: string;
	
	    static createFrom(source: any = {}) {
	        return new SaveResult(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.success = source["success"];
	        this.filePath = source["filePath"];
	        this.error = source["error"];
	    }
	}
	export class SubscriptionConfig {
	    trigger: boolean;
	    snapshot: boolean;
	
	    static createFrom(source: any = {}) {
	        return new SubscriptionConfig(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.trigger = source["trigger"];
	        this.snapshot = source["snapshot"];
	    }
	}

}

